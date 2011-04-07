// Copyright Maarten L. Hekkelman, Radboud University 2008-2011.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)
//
// webapp is a base class used to construct web applications in C++ using libzeep
//

#pragma once

#include <map>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

#include <zeep/exception.hpp>
#include <zeep/http/request.hpp>
#include <zeep/http/server.hpp>

// --------------------------------------------------------------------
//

namespace zeep {
namespace http {

namespace fs = boost::filesystem;

namespace el { class scope; class object; }

class parameter_value
{
  public:
				parameter_value()
					: m_defaulted(false) {}
				parameter_value(
					const std::string&	v,
					bool				defaulted)
					: m_v(v)
					, m_defaulted(defaulted) {}

	template<class T>
	T			as() const;
	bool		empty() const								{ return m_v.empty(); }
	bool		defaulted() const							{ return m_defaulted; }

  private:
	std::string	m_v;
	bool		m_defaulted;
};

class parameter_map : public std::map<std::string, parameter_value>
{
  public:
	
	// add a name/value pair as a string formatted as 'name=value'
	void		add(
					const std::string&	param);

	void		add(
					std::string			name,
					std::string			value);

	template<class T>
	const parameter_value&
				get(
					const std::string&	name,
					T					defaultValue);
};

extern const std::string kLibZeepWebAppNS;

class webapp : public http::server
{
  public:
					// first parameter to constructor is the
					// namespace to use in template XHTML files.
					webapp(
						const std::string&	ns,
						const fs::path&		docroot = ".");

	virtual			~webapp();

	virtual void	set_docroot(
						const fs::path&		docroot);
	fs::path		get_docroot() const		{ return m_docroot; }
	
  protected:
	
	virtual void	handle_request(
						const request&		req,
						reply&				rep);

	// webapp works with 'handlers' that are methods 'mounted' on a path in the requested URI
	
	typedef boost::function<void(const request& request, const el::scope& scope, reply& reply)> handler_type;

	void			mount(const std::string& path, handler_type handler);

	// only one handler defined here
	virtual void	handle_file(
						const request&		request,
						const el::scope&	scope,
						reply&				reply);

	// Use load_template to fetch the XHTML template file
	virtual void	load_template(
						const std::string&	file,
						xml::document&		doc);

	void			load_template(
						const fs::path&		file,
						xml::document&		doc)
					{
						load_template(file.string(), doc);
					}

	// create a reply based on a template
	virtual void	create_reply_from_template(
						const std::string&	file,
						const el::scope&	scope,
						reply&				reply);
	
	// process xml parses the XHTML and fills in the special tags and evaluates the el constructs
	virtual void	process_xml(
						xml::node*			node,
						const el::scope&	scope,
						fs::path			dir);

	typedef boost::function<void(xml::element* node, const el::scope& scope, fs::path dir)> processor_type;

	virtual void	add_processor(
						const std::string&	name,
						processor_type		processor);

	virtual void	process_include(
						xml::element*		node,
						const el::scope&	scope,
						fs::path			dir);

	virtual void	process_if(
						xml::element*		node,
						const el::scope&	scope,
						fs::path			dir);

	virtual void	process_iterate(
						xml::element*		node,
						const el::scope&	scope,
						fs::path			dir);

	virtual void	process_for(
						xml::element*		node,
						const el::scope&	scope,
						fs::path			dir);

	virtual void	process_number(
						xml::element*		node,
						const el::scope&	scope,
						fs::path			dir);

	virtual void	process_options(
						xml::element*		node,
						const el::scope&	scope,
						fs::path			dir);

	virtual void	process_option(
						xml::element*		node,
						const el::scope&	scope,
						fs::path			dir);

	virtual void	process_checkbox(
						xml::element*		node,
						const el::scope&	scope,
						fs::path			dir);

	virtual void	process_url(
						xml::element*		node,
						const el::scope&	scope,
						fs::path			dir);

	virtual void	process_param(
						xml::element*		node,
						const el::scope&	scope,
						fs::path			dir);

	virtual void	process_embed(
						xml::element*		node,
						const el::scope&	scope,
						fs::path			dir);

	virtual void	init_scope(
						el::scope&			scope);

	virtual void	get_cookies(
						const el::scope&	scope,
						parameter_map&		cookies);

	virtual void	get_parameters(
						const el::scope&	scope,
						parameter_map&		parameters);

  private:
	typedef std::map<std::string,handler_type>		handler_map;
	typedef std::map<std::string,processor_type>	processor_map;
	
	std::string		m_ns;
	fs::path		m_docroot;
	handler_map		m_dispatch_table;
	processor_map	m_processor_table;
};

template<class T>
inline
T parameter_value::as() const
{
	return boost::lexical_cast<T>(m_v);
}

template<>
inline
std::string parameter_value::as<std::string>() const
{
	return m_v;
}

template<>
inline
bool parameter_value::as<bool>() const
{
	bool result = false;
	
	if (not m_v.empty() and m_v != "false")
	{
		if (m_v == "true")
			result = true;
		else
		{
			try { result = boost::lexical_cast<int>(m_v) != 0; } catch (...) {}
		}
	}
	
	return result;
}

template<class T>
inline
const parameter_value&
parameter_map::get(
	const std::string&	name,
	T					defaultValue)
{
	if (count(name) == 0)
		insert(std::make_pair(name, parameter_value(boost::lexical_cast<std::string>(defaultValue), true)));
	return operator[](name);
}

// specialisation for const char*
template<>
inline
const parameter_value&
parameter_map::get(
	const std::string&	name,
	const char*			defaultValue)
{
	if (defaultValue == nil)
		defaultValue = "";
	
	if (count(name) == 0 or operator[](name).empty())
		insert(std::make_pair(name, parameter_value(defaultValue, true)));
	return operator[](name);
}

// specialisation for bool (if missing, value is false)
template<>
inline
const parameter_value&
parameter_map::get(
	const std::string&	name,
	bool				defaultValue)
{
	if (count(name) == 0 or operator[](name).empty())
		insert(std::make_pair(name, parameter_value("false", true)));
	return operator[](name);
}

}
}

