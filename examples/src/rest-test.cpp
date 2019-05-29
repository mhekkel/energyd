//               Copyright Maarten L. Hekkelman.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <functional>
#include <tuple>

#include <boost/algorithm/string.hpp>

#include <zeep/http/webapp.hpp>
#include <zeep/http/md5.hpp>

#include <boost/filesystem.hpp>

using namespace std;
namespace zh = zeep::http;
namespace el = zeep::el;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;

using json = el::element;

struct TellerStand
{
	int	tellerId;
	float stand;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long version)
	{
		ar & zeep::make_nvp("tellerId", tellerId)
		   & zeep::make_nvp("stand", stand);
	}
};

struct Opname
{
	string						id;
	boost::posix_time::ptime	datum;
	vector<TellerStand> 		standen;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long version)
	{
		ar & zeep::make_nvp("id", id)
		   & zeep::make_nvp("datum", datum)
		   & zeep::make_nvp("standen", standen);
	}
};

class Controller
{
  public:
	Controller(const string& path)
		: m_path(path)
	{

	}

	virtual void handle_request(const zh::request& req, zh::reply& rep)
	{

	}

	constexpr const std::string& get_path() const		{ return m_path; } 

  protected:
	string m_path;
};

class RestController : public Controller
{
  public:
	RestController(const string& path)
		: Controller(path)
	{
	}

	virtual void handle_request(const zh::request& req, zh::reply& rep);

  protected:

	struct mount_point_base
	{
		mount_point_base(const char* path, zh::method_type method)
			: m_path(path), m_method(method) {}

		virtual void call(const zh::request& req, zh::reply& reply) = 0;

		std::string m_path;
		zh::method_type m_method;
	};

	template<typename Callback, typename...>
	struct mount_point {};

	template<typename ControllerType, typename Result, typename... Args>
	struct mount_point<Result(ControllerType::*)(Args...)> : mount_point_base
	{
		using Sig = Result(ControllerType::*)(Args...);
		using ArgsTuple = std::tuple<Args...>;
		using Callback = std::function<Result(Args...)>;

		static constexpr size_t N = sizeof...(Args);

		template<typename... Names>
		mount_point(const char* path, zh::method_type method, RestController* owner, Sig sig, Names... names)
			: mount_point_base(path, method)
		{
			static_assert(sizeof...(Names) == sizeof...(Args), "Number of names should be equal to number of arguments of callback function");
			
			ControllerType* controller = dynamic_cast<ControllerType*>(owner);
			if (controller == nullptr)
				throw std::runtime_error("Invalid controller for callback");

			m_callback = [controller, sig](Args... args) {
				return (controller->*sig)(args...);
			};
			// for (auto name: {...names })
			size_t i = 0;
			for (auto name: {names...})
				m_names[i++] = name;
		}

		virtual void call(const zh::request& req, zh::reply& reply)
		{
			ArgsTuple args = collect_arguments(req, std::make_index_sequence<N>());

			json result = std::apply(m_callback, args);

			reply.set_content(result);
		}

		template<std::size_t... I>
		ArgsTuple collect_arguments(const zh::request& req, std::index_sequence<I...>)
		{
			return std::make_tuple(req.get_parameter(m_names[I])...);
		}

		Callback m_callback;
		std::array<const char*, N>	m_names;
	};

	template<typename Callback, typename... ArgNames>
	void map_request(const char* mountPoint, zh::method_type method, Callback callback, ArgNames... names)
	{
		m_mountpoints.emplace_back(new mount_point<Callback>(mountPoint, method, this, callback, names...));
	}

	template<typename Callback, typename... ArgNames>
	void map_post_request(const char* mountPoint, Callback callback, ArgNames... names)
	{
		map_request(mountPoint, zh::method_type::POST, callback, names...);
	}

	template<typename Sig, typename... ArgNames>
	void map_get_request(const char* mountPoint, Sig callback, ArgNames... names)
	{
		map_request(mountPoint, zh::method_type::GET, callback, names...);
	}

	std::list<mount_point_base*> m_mountpoints;
};

void RestController::handle_request(const zh::request& req, zh::reply& rep)
{
	string p = req.uri;

	if (p.front() == '/')
		p.erase(0, 1);

	if (ba::starts_with(p, m_path))
		p.erase(0, m_path.length());
	
	if (p.front() == '/')
		p.erase(0, 1);

	auto mp = find_if(m_mountpoints.begin(), m_mountpoints.end(),
		[&](auto e) { return e->m_method == req.method and e->m_path == p; });
	

	if (mp == m_mountpoints.end())
		rep = zh::reply::stock_reply(zh::not_found, "No handler for rest call to " + p);
	else
		(*mp)->call(req, rep);
}

class my_rest_controller : public RestController
{
  public:
	my_rest_controller()
		: RestController("ajax")
	{
		map_post_request("opname", &my_rest_controller::post_opname, "data");
		// map_get_request("opname", &my_rest_controller::get_opname, "id");
	}

	// CRUD routines
	std::string post_opname(std::string opname)
	{
		cout << "post_opname aangeroepen met parameter " << opname << endl;
		return "hello, world!";
	}


	// Opname get_opname(std::string id);
	// json put_opname(std::string id, Opname&& opname);
	// json delete_opname(std::string id);

};


class my_first_rest_server : public zh::webapp
{
  public:

	my_first_rest_server()
		: webapp("http://www.hekkelman.com/libzeep/ml", fs::current_path() / "docroot")
	{
		mount(m_controller.get_path(), &my_first_rest_server::handle_call);
	}

  protected:
	virtual void handle_call(const zh::request& req, const el::scope& scope, zh::reply& rep)
	{
		m_controller.handle_request(req, rep);
	}

  private:
	my_rest_controller m_controller;
};


int main()
{
	my_first_rest_server app;

	app.bind("0.0.0.0", 10333);
    thread t(bind(&my_first_rest_server::run, &app, 2));
	t.join();

	return 0;
}