// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#include <zeep/config.hpp>

#include <map>
#include <set>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/random/random_device.hpp>

#include <boost/format.hpp>

#include <zeep/http/webapp.hpp>
#include <zeep/xml/unicode_support.hpp>
#include <zeep/http/webapp/el.hpp>
#include <zeep/http/md5.hpp>

namespace ba = boost::algorithm;
namespace io = boost::iostreams;
namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

namespace zeep
{
namespace http
{

// --------------------------------------------------------------------
//

// add a name/value pair as a std::string formatted as 'name=value'
void parameter_map::add(const std::string& param)
{
	std::string name, value;

	std::string::size_type d = param.find('=');
	if (d != std::string::npos)
	{
		name = param.substr(0, d);
		value = param.substr(d + 1);
	}

	add(name, value);
}

void parameter_map::add(std::string name, std::string value)
{
	name = decode_url(name);
	if (not value.empty())
		value = decode_url(value);

	insert(make_pair(name, parameter_value(value, false)));
}

void parameter_map::replace(std::string name, std::string value)
{
	if (count(name))
		erase(lower_bound(name), upper_bound(name));
	add(name, value);
}

// --------------------------------------------------------------------
//

struct auth_info
{
	auth_info(const std::string& realm);

	bool validate(const std::string& method, const std::string& uri, const std::string& ha1, std::map<std::string, std::string>& info);

	std::string get_challenge() const;
	bool stale() const;

	std::string m_nonce, m_realm;
	std::set<uint32_t> m_replay_check;
	pt::ptime m_created;
};

auth_info::auth_info(const std::string& realm)
	: m_realm(realm)
{
	using namespace boost::gregorian;

	boost::random::random_device rng;
	uint32_t data[4] = {rng(), rng(), rng(), rng()};

	m_nonce = md5(data, sizeof(data)).finalise();
	m_created = pt::second_clock::local_time();
}

std::string auth_info::get_challenge() const
{
	std::string challenge = "Digest ";
	challenge += "realm=\"" + m_realm + "\", qop=\"auth\", nonce=\"" + m_nonce + '"';
	return challenge;
}

bool auth_info::stale() const
{
	pt::time_duration age = pt::second_clock::local_time() - m_created;
	return age.total_seconds() > 1800;
}

bool auth_info::validate(const std::string& method, const std::string& uri, const std::string& ha1, std::map<std::string, std::string>& info)
{
	bool valid = false;

	uint32_t nc = strtol(info["nc"].c_str(), nullptr, 16);
	if (not m_replay_check.count(nc))
	{
		std::string ha2 = md5(method + ':' + info["uri"]).finalise();

		std::string response = md5(
								   ha1 + ':' +
								   info["nonce"] + ':' +
								   info["nc"] + ':' +
								   info["cnonce"] + ':' +
								   info["qop"] + ':' +
								   ha2)
								   .finalise();

		valid = info["response"] == response;

		// keep a list of seen nc-values in m_replay_check
		m_replay_check.insert(nc);
	}
	return valid;
}

// --------------------------------------------------------------------
//

basic_webapp::basic_webapp(const std::string& ns, const fs::path& docroot)
	: m_ns(ns), m_docroot(docroot)
{
	m_processor_table["include"] = bind(&basic_webapp::process_include, this, _1, _2, _3);
	m_processor_table["if"] = bind(&basic_webapp::process_if, this, _1, _2, _3);
	m_processor_table["iterate"] = bind(&basic_webapp::process_iterate, this, _1, _2, _3);
	m_processor_table["for"] = bind(&basic_webapp::process_for, this, _1, _2, _3);
	m_processor_table["number"] = bind(&basic_webapp::process_number, this, _1, _2, _3);
	m_processor_table["options"] = bind(&basic_webapp::process_options, this, _1, _2, _3);
	m_processor_table["option"] = bind(&basic_webapp::process_option, this, _1, _2, _3);
	m_processor_table["checkbox"] = bind(&basic_webapp::process_checkbox, this, _1, _2, _3);
	m_processor_table["url"] = bind(&basic_webapp::process_url, this, _1, _2, _3);
	m_processor_table["param"] = bind(&basic_webapp::process_param, this, _1, _2, _3);
	m_processor_table["embed"] = bind(&basic_webapp::process_embed, this, _1, _2, _3);
}

basic_webapp::~basic_webapp()
{
}

void basic_webapp::handle_request(const request& req, reply& rep)
{
	std::string uri = req.uri;

	// shortcut, only handle GET, POST and PUT
	if (req.method != "GET" and req.method != "POST" and req.method != "PUT" and
		req.method != "OPTIONS" and req.method != "HEAD")
	{
		create_error_reply(req, bad_request, rep);
		return;
	}

	try
	{
		// start by sanitizing the request's URI, first parse the parameters
		std::string ps = req.payload;
		if (req.method != "POST")
		{
			std::string::size_type d = uri.find('?');
			if (d != std::string::npos)
			{
				ps = uri.substr(d + 1);
				uri.erase(d, std::string::npos);
			}
		}

		// strip off the http part including hostname and such
		if (ba::starts_with(uri, "http://"))
		{
			std::string::size_type s = uri.find_first_of('/', 7);
			if (s != std::string::npos)
				uri.erase(0, s);
		}

		// now make the path relative to the root
		while (uri.length() > 0 and uri[0] == '/')
			uri.erase(uri.begin());

		// decode the path elements
		std::string action = uri;
		std::string::size_type s = action.find('/');
		if (s != std::string::npos)
			action.erase(s, std::string::npos);

		// set up the scope by putting some globals in it
		el::scope scope(req);

		scope.put("action", el::object(action));
		scope.put("uri", el::object(uri));
		s = uri.find('?');
		if (s != std::string::npos)
			uri.erase(s, std::string::npos);
		scope.put("baseuri", uri);
		scope.put("mobile", req.is_mobile());

		auto handler = find_if(m_dispatch_table.begin(), m_dispatch_table.end(),
							   [uri](const mount_point& m) -> bool { return m.path == uri; });

		if (handler == m_dispatch_table.end())
		{
			handler = find_if(m_dispatch_table.begin(), m_dispatch_table.end(),
							  [action](const mount_point& m) -> bool { return m.path == action; });
		}

		if (handler != m_dispatch_table.end())
		{
			if (req.method == "OPTIONS")
			{
				rep = reply::stock_reply(ok);
				rep.set_header("Allow", "GET,HEAD,POST,OPTIONS");
				rep.set_content("", "text/plain");
			}
			else
			{
				// Do authentication here, if needed
				if (not handler->realm.empty())
				{
					// #pragma message("This sucks, please fix")
					// this is extremely dirty...
					auto& r = const_cast<request& >(req);
					validate_authentication(r, handler->realm);

					scope.put("username", req.username);
				}

				init_scope(scope);

				handler->handler(req, scope, rep);

				if (req.method == "HEAD")
					rep.set_content("", rep.get_content_type());
			}
		}
		else
			throw not_found;
	}
	catch (unauthorized_exception& e)
	{
		create_unauth_reply(req, e.m_stale, e.m_realm, rep);
	}
	catch (status_type& s)
	{
		create_error_reply(req, s, rep);
	}
	catch (std::exception& e)
	{
		create_error_reply(req, internal_server_error, e.what(), rep);
	}
}

void basic_webapp::create_unauth_reply(const request& req, bool stale, const std::string& realm, const std::string& authenticate, reply& rep)
{
	std::unique_lock<std::mutex> lock(m_auth_mutex);

	create_error_reply(req, unauthorized, get_status_text(unauthorized), rep);

	m_auth_info.push_back(auth_info(realm));

	std::string challenge = m_auth_info.back().get_challenge();
	if (stale)
		challenge += ", stale=\"true\"";

	rep.set_header(authenticate, challenge);
}

void basic_webapp::create_error_reply(const request& req, status_type status, reply& rep)
{
	create_error_reply(req, status, "", rep);
}

void basic_webapp::create_error_reply(const request& req, status_type status, const std::string& message, reply& rep)
{
	el::scope scope(req);

	el::object error;
	error["nr"] = static_cast<int>(status);
	error["head"] = get_status_text(status);
	error["description"] = get_status_description(status);

	if (not message.empty())
		error["message"] = message;

	el::object request;
	request["line"] =
		ba::starts_with(req.uri, "http://") ? (boost::format("%1% %2% HTTP%3%/%4%") % req.method % req.uri % req.http_version_major % req.http_version_minor).str() : (boost::format("%1% http://%2%%3% HTTP%4%/%5%") % req.method % req.get_header("Host") % req.uri % req.http_version_major % req.http_version_minor).str();
	request["username"] = req.username;
	error["request"] = request;

	scope.put("error", error);

	create_reply_from_template("error.html", scope, rep);
	rep.set_status(status);
}

void basic_webapp::handle_file(
	const zeep::http::request& request,
	const el::scope& scope,
	zeep::http::reply& reply)
{
	using namespace boost::local_time;
	using namespace boost::posix_time;

	fs::path file = get_docroot() / scope["baseuri"].as<std::string>();
	if (not fs::exists(file))
	{
		reply = zeep::http::reply::stock_reply(not_found);
		return;
	}

	std::string ifModifiedSince;
	for (const zeep::http::header& h : request.headers)
	{
		if (ba::iequals(h.name, "If-Modified-Since"))
		{
			local_date_time modifiedSince(local_sec_clock::local_time(time_zone_ptr()));

			local_time_input_facet *lif1(new local_time_input_facet("%a, %d %b %Y %H:%M:%S GMT"));

			std::stringstream ss;
			ss.imbue(std::locale(std::locale::classic(), lif1));
			ss.str(h.value);
			ss >> modifiedSince;

			local_date_time fileDate(from_time_t(fs::last_write_time(file)), time_zone_ptr());

			if (fileDate <= modifiedSince)
			{
				reply = zeep::http::reply::stock_reply(zeep::http::not_modified);
				return;
			}

			break;
		}
	}

	fs::ifstream in(file, std::ios::binary);
	std::stringstream out;

	io::copy(in, out);

	std::string mimetype = "text/plain";

	if (file.extension() == ".css")
		mimetype = "text/css";
	else if (file.extension() == ".js")
		mimetype = "text/javascript";
	else if (file.extension() == ".png")
		mimetype = "image/png";
	else if (file.extension() == ".svg")
		mimetype = "image/svg+xml";
	else if (file.extension() == ".html" or file.extension() == ".htm")
		mimetype = "text/html";
	else if (file.extension() == ".xml" or file.extension() == ".xsl" or file.extension() == ".xslt")
		mimetype = "text/xml";
	else if (file.extension() == ".xhtml")
		mimetype = "application/xhtml+xml";

	reply.set_content(out.str(), mimetype);

	local_date_time t(local_sec_clock::local_time(time_zone_ptr()));
	local_time_facet *lf(new local_time_facet("%a, %d %b %Y %H:%M:%S GMT"));

	std::stringstream s;
	s.imbue(std::locale(std::cout.getloc(), lf));

	ptime pt = from_time_t(boost::filesystem::last_write_time(file));
	local_date_time t2(pt, time_zone_ptr());
	s << t2;

	reply.set_header("Last-Modified", s.str());
}

void basic_webapp::get_cookies(
	const el::scope& scope,
	parameter_map& cookies)
{
	const request& req = scope.get_request();
	for (const header& h : req.headers)
	{
		if (h.name != "Cookie")
			continue;

		std::vector<std::string> rawCookies;
		ba::split(rawCookies, h.value, ba::is_any_of(";"));

		for (std::string& cookie : rawCookies)
		{
			ba::trim(cookie);
			cookies.add(cookie);
		}
	}
}

void basic_webapp::set_docroot(const fs::path& path)
{
	m_docroot = path;
}

void basic_webapp::load_template(const std::string& file, xml::document& doc)
{
	fs::ifstream data(m_docroot / file, std::ios::binary);
	if (not data.is_open())
	{
		if (not fs::exists(m_docroot))
			throw exception((boost::format("configuration error, docroot not found: '%1%'") % m_docroot).str());
		else
		{
#if defined(_MSC_VER)
			char msg[1024] = "";

			DWORD dw = ::GetLastError();
			if (dw != NO_ERROR)
			{
				char *lpMsgBuf = nullptr;
				int m = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
										 NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpMsgBuf, 0, NULL);

				if (lpMsgBuf != nullptr)
				{
					// strip off the trailing whitespace characters
					while (m > 0 and isspace(lpMsgBuf[m - 1]))
						--m;
					lpMsgBuf[m] = 0;

					strncpy(msg, lpMsgBuf, sizeof(msg));

					::LocalFree(lpMsgBuf);
				}
			}

			throw exception((boost::format("error opening: %1% (%2%)") % (m_docroot / file) % msg).str());
#else
			throw exception((boost::format("error opening: %1% (%2%)") % (m_docroot / file) % strerror(errno)).str());
#endif
		}
	}
	doc.read(data);
}

void basic_webapp::create_reply_from_template(const std::string& file, const el::scope& scope, reply& reply)
{
	xml::document doc;
	doc.set_preserve_cdata(true);

	load_template(file, doc);

	xml::element *root = doc.child();
	process_xml(root, scope, "/");
	reply.set_content(doc);
}

void basic_webapp::process_xml(xml::node *node, const el::scope& scope, fs::path dir)
{
	xml::text *text = dynamic_cast<xml::text *>(node);

	if (text != nullptr)
	{
		std::string s = text->str();
		if (el::process_el(scope, s))
			text->str(s);
		return;
	}

	xml::element *e = dynamic_cast<xml::element *>(node);
	if (e == nullptr)
		return;

	// if node is one of our special nodes, we treat it here
	if (e->ns() == m_ns)
	{
		xml::container *parent = e->parent();

		try
		{
			el::scope nested(scope);

			processor_map::iterator p = m_processor_table.find(e->name());
			if (p != m_processor_table.end())
				p->second(e, scope, dir);
			else
				throw exception((boost::format("unimplemented <mrs:%1%> tag") % e->name()).str());
		}
		catch (exception& ex)
		{
			xml::node *replacement = new xml::text(
				(boost::format("Error processing directive 'mrs:%1%': %2%") %
				 e->name() % ex.what())
					.str());

			parent->insert(e, replacement);
		}

		try
		{
			//			assert(parent == e->parent());
			//			assert(find(parent->begin(), parent->end(), e) != parent->end());

			parent->remove(e);
			delete e;
		}
		catch (exception& ex)
		{
			std::cerr << "exception: " << ex.what() << std::endl
					  << *e << std::endl;
		}
	}
	else
	{
		for (xml::attribute& a : boost::iterator_range<xml::element::attribute_iterator>(e->attr_begin(), e->attr_end()))
		{
			std::string s = a.value();
			if (process_el(scope, s))
				a.value(s);
		}

		std::list<xml::node *> nodes;
		copy(e->node_begin(), e->node_end(), back_inserter(nodes));

		for (xml::node *n : nodes)
		{
			process_xml(n, scope, dir);
		}
	}
}

void basic_webapp::add_processor(const std::string& name, processor_type processor)
{
	m_processor_table[name] = processor;
}

void basic_webapp::process_include(xml::element *node, const el::scope& scope, fs::path dir)
{
	// an include directive, load file and include resulting content
	std::string file = node->get_attribute("file");
	process_el(scope, file);

	if (file.empty())
		throw exception("missing file attribute");

	xml::document doc;
	doc.set_preserve_cdata(true);
	load_template(dir / file, doc);

	xml::element *replacement = doc.child();
	doc.root()->remove(replacement);

	xml::container *parent = node->parent();
	parent->insert(node, replacement);

	process_xml(replacement, scope, (dir / file).parent_path());
}

void basic_webapp::process_if(xml::element *node, const el::scope& scope, fs::path dir)
{
	std::string test = node->get_attribute("test");
	if (evaluate_el(scope, test))
	{
		for (xml::node *c : node->nodes())
		{
			xml::node *clone = c->clone();

			xml::container *parent = node->parent();
			assert(parent);

			parent->insert(node, clone); // insert before processing, to assign namespaces
			process_xml(clone, scope, dir);
		}
	}
}

void basic_webapp::process_iterate(xml::element *node, const el::scope& scope, fs::path dir)
{
	using detail::value_type;

	el::object collection = scope[node->get_attribute("collection")];
	if (collection.type() != value_type::array)
		evaluate_el(scope, node->get_attribute("collection"), collection);

	std::string var = node->get_attribute("var");
	if (var.empty())
		throw exception("missing var attribute in mrs:iterate");

	for (el::object& o : collection)
	{
		el::scope s(scope);
		s.put(var, o);

		for (xml::node *c : node->nodes())
		{
			xml::node *clone = c->clone();

			xml::container *parent = node->parent();
			assert(parent);

			parent->insert(node, clone); // insert before processing, to assign namespaces
			process_xml(clone, s, dir);
		}
	}
}

void basic_webapp::process_for(xml::element *node, const el::scope& scope, fs::path dir)
{
	el::object b, e;

	evaluate_el(scope, node->get_attribute("begin"), b);
	evaluate_el(scope, node->get_attribute("end"), e);

	std::string var = node->get_attribute("var");
	if (var.empty())
		throw exception("missing var attribute in mrs:iterate");

	for (int32_t i = b.as<int32_t>(); i <= e.as<int32_t>(); ++i)
	{
		el::scope s(scope);
		s.put(var, el::object(i));

		for (xml::node *c : node->nodes())
		{
			xml::container *parent = node->parent();
			assert(parent);
			xml::node *clone = c->clone();

			parent->insert(node, clone); // insert before processing, to assign namespaces
			process_xml(clone, s, dir);
		}
	}
}

class with_thousands : public std::numpunct<char>
{
protected:
	//	char_type do_thousands_sep() const	{ return tsp; }
	std::string do_grouping() const { return "\03"; }
	//	char_type do_decimal_point() const	{ return dsp; }
};

void basic_webapp::process_number(xml::element *node, const el::scope& scope, fs::path dir)
{
	std::string number = node->get_attribute("n");
	std::string format = node->get_attribute("f");

	if (format == "#,##0B") // bytes, convert to a human readable form
	{
		const char kBase[] = {'B', 'K', 'M', 'G', 'T', 'P', 'E'}; // whatever

		el::object n;
		evaluate_el(scope, number, n);

		uint64_t nr = n.as<uint64_t>();
		int base = 0;

		while (nr > 1024)
		{
			nr /= 1024;
			++base;
		}

		std::locale mylocale(std::locale(), new with_thousands);

		std::ostringstream s;
		s.imbue(mylocale);
		s.setf(std::ios::fixed, std::ios::floatfield);
		s.precision(1);
		s << nr << ' ' << kBase[base];
		number = s.str();
	}
	else if (format.empty() or ba::starts_with(format, "#,##0"))
	{
		el::object n;
		evaluate_el(scope, number, n);

		uint64_t nr = n.as<uint64_t>();

		std::locale mylocale(std::locale(), new with_thousands);

		std::ostringstream s;
		s.imbue(mylocale);
		s << nr;
		number = s.str();
	}

	zeep::xml::node *replacement = new zeep::xml::text(number);

	zeep::xml::container *parent = node->parent();
	parent->insert(node, replacement);
}

void basic_webapp::process_options(xml::element *node, const el::scope& scope, fs::path dir)
{
	using ::zeep::detail::value_type;

	el::object collection = scope[node->get_attribute("collection")];
	if (collection.type() != value_type::array)
		evaluate_el(scope, node->get_attribute("collection"), collection);

	std::string value = node->get_attribute("value");
	std::string label = node->get_attribute("label");

	std::string selected = node->get_attribute("selected");
	if (not selected.empty())
	{
		el::object o;
		evaluate_el(scope, selected, o);
		selected = o.as<std::string>();
	}

	for (el::object& o : collection)
	{
		zeep::xml::element *option = new zeep::xml::element("option");

		if (not(value.empty() or label.empty()))
		{
			option->set_attribute("value", o[value].as<std::string>());
			if (selected == o[value].as<std::string>())
				option->set_attribute("selected", "selected");
			option->add_text(o[label].as<std::string>());
		}
		else
		{
			option->set_attribute("value", o.as<std::string>());
			if (selected == o.as<std::string>())
				option->set_attribute("selected", "selected");
			option->add_text(o.as<std::string>());
		}

		zeep::xml::container *parent = node->parent();
		assert(parent);
		parent->insert(node, option);
	}
}

void basic_webapp::process_option(xml::element *node, const el::scope& scope, fs::path dir)
{
	std::string value = node->get_attribute("value");
	if (not value.empty())
	{
		el::object o;
		evaluate_el(scope, value, o);
		value = o.as<std::string>();
	}

	std::string selected = node->get_attribute("selected");
	if (not selected.empty())
	{
		el::object o;
		evaluate_el(scope, selected, o);
		selected = o.as<std::string>();
	}

	zeep::xml::element *option = new zeep::xml::element("option");

	option->set_attribute("value", value);
	if (selected == value)
		option->set_attribute("selected", "selected");

	zeep::xml::container *parent = node->parent();
	assert(parent);
	parent->insert(node, option);

	for (zeep::xml::node *c : node->nodes())
	{
		zeep::xml::node *clone = c->clone();
		option->push_back(clone);
		process_xml(clone, scope, dir);
	}
}

void basic_webapp::process_checkbox(xml::element *node, const el::scope& scope, fs::path dir)
{
	std::string name = node->get_attribute("name");
	if (not name.empty())
	{
		el::object o;
		evaluate_el(scope, name, o);
		name = o.as<std::string>();
	}

	bool checked = false;
	if (not node->get_attribute("checked").empty())
	{
		el::object o;
		evaluate_el(scope, node->get_attribute("checked"), o);
		checked = o.as<bool>();
	}

	zeep::xml::element *checkbox = new zeep::xml::element("input");
	checkbox->set_attribute("type", "checkbox");
	checkbox->set_attribute("name", name);
	checkbox->set_attribute("value", "true");
	if (checked)
		checkbox->set_attribute("checked", "true");

	zeep::xml::container *parent = node->parent();
	assert(parent);
	parent->insert(node, checkbox);

	for (zeep::xml::node *c : node->nodes())
	{
		zeep::xml::node *clone = c->clone();
		checkbox->push_back(clone);
		process_xml(clone, scope, dir);
	}
}

void basic_webapp::process_url(xml::element *node, const el::scope& scope, fs::path dir)
{
	std::string var = node->get_attribute("var");

	parameter_map parameters;
	get_parameters(scope, parameters);

	for (zeep::xml::element *e : *node)
	{
		if (e->ns() == m_ns and e->name() == "param")
		{
			std::string name = e->get_attribute("name");
			std::string value = e->get_attribute("value");

			process_el(scope, value);
			parameters.replace(name, value);
		}
	}

	std::string url = scope["baseuri"].as<std::string>();

	bool first = true;
	for (parameter_map::value_type p : parameters)
	{
		if (first)
			url += '?';
		else
			url += '&';
		first = false;

		url += zeep::http::encode_url(p.first) + '=' + zeep::http::encode_url(p.second.as<std::string>());
	}

	el::scope& s(const_cast<el::scope& >(scope));
	s.put(var, url);
}

void basic_webapp::process_param(xml::element *node, const el::scope& scope, fs::path dir)
{
	throw exception("Invalid XML, cannot have a stand-alone mrs:param element");
}

void basic_webapp::process_embed(xml::element *node, const el::scope& scope, fs::path dir)
{
	// an embed directive, load xml from attribute and include parsed content
	std::string xml = scope[node->get_attribute("var")].as<std::string>();

	if (xml.empty())
		throw exception("Missing var attribute in embed tag");

	zeep::xml::document doc;
	doc.set_preserve_cdata(true);
	doc.read(xml);

	zeep::xml::element *replacement = doc.child();
	doc.root()->remove(replacement);

	zeep::xml::container *parent = node->parent();
	parent->insert(node, replacement);

	process_xml(replacement, scope, dir);
}

void basic_webapp::init_scope(el::scope& scope)
{
}

void basic_webapp::get_parameters(const el::scope& scope, parameter_map& parameters)
{
	const request& req = scope.get_request();

	std::string ps;

	if (req.method == "POST")
	{
		std::string contentType = req.get_header("Content-Type");

		if (ba::starts_with(contentType, "application/x-www-form-urlencoded"))
			ps = req.payload;
		//		else
		//		{
		//
		//		}
	}
	else if (req.method == "GET" or req.method == "PUT")
	{
		std::string::size_type d = req.uri.find('?');
		if (d != std::string::npos)
			ps = req.uri.substr(d + 1);
	}

	while (not ps.empty())
	{
		std::string::size_type e = ps.find_first_of("&;");
		std::string param;

		if (e != std::string::npos)
		{
			param = ps.substr(0, e);
			ps.erase(0, e + 1);
		}
		else
			swap(param, ps);

		if (not param.empty())
			parameters.add(param);
	}
}

// --------------------------------------------------------------------
//

std::string basic_webapp::validate_authentication(const std::string& authorization,
												  const std::string& method, const std::string& uri, const std::string& realm)
{
	if (authorization.empty())
		throw unauthorized_exception(false, realm);

	// That was easy, now check the response

	std::map<std::string, std::string> info;

	boost::regex re("(\\w+)=(?|\"([^\"]*)\"|'([^']*)'|(\\w+))(?:,\\s*)?");
	const char *b = authorization.c_str();
	const char *e = b + authorization.length();
	boost::match_results<const char *> m;
	while (b < e and boost::regex_search(b, e, m, re))
	{
		info[std::string(m[1].first, m[1].second)] = std::string(m[2].first, m[2].second);
		b = m[0].second;
	}

	if (info["realm"] != realm)
		throw unauthorized_exception(false, realm);

	std::string ha1 = get_hashed_password(info["username"], realm);

	// lock to avoid accessing m_auth_info from multiple threads at once
	std::unique_lock<std::mutex> lock(m_auth_mutex);
	bool authorized = false, stale = false;

	for (auto auth = m_auth_info.begin(); auth != m_auth_info.end(); ++auth)
	{
		if (auth->m_realm == realm and auth->m_nonce == info["nonce"] and auth->validate(method, uri, ha1, info))
		{
			authorized = true;
			stale = auth->stale();
			if (stale)
				m_auth_info.erase(auth);
			break;
		}
	}

	if (stale or not authorized)
		throw unauthorized_exception(stale, realm);

	return info["username"];
}

std::string basic_webapp::get_hashed_password(const std::string& username, const std::string& realm)
{
	return "";
}

// --------------------------------------------------------------------
//

webapp::webapp(const std::string& ns, const boost::filesystem::path& docroot)
	: basic_webapp(ns, docroot)
{
}

webapp::~webapp()
{
}

void webapp::handle_request(const request& req, reply& rep)
{
	server::log() << req.uri;
	basic_webapp::handle_request(req, rep);
}

} // namespace http
} // namespace zeep
