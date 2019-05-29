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
#include <zeep/el/parser.hpp>
#include <zeep/http/rest-controller.hpp>

using namespace std;
namespace zh = zeep::http;
namespace el = zeep::el;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;

using json = el::element;

// struct TellerStand
// {
// 	int	tellerId;
// 	float stand;

// 	template<typename Archive>
// 	void serialize(Archive& ar, unsigned long version)
// 	{
// 		ar & zeep::make_nvp("tellerId", tellerId)
// 		   & zeep::make_nvp("stand", stand);
// 	}
// };

// struct Opname
// {
// 	string						id;
// 	boost::posix_time::ptime	datum;
// 	vector<TellerStand> 		standen;

// 	template<typename Archive>
// 	void serialize(Archive& ar, unsigned long version)
// 	{
// 		ar & zeep::make_nvp("id", id)
// 		   & zeep::make_nvp("datum", datum)
// 		   & zeep::make_nvp("standen", standen);
// 	}
// };


struct Antwoord
{
	string	boodschap;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long)
	{
		ar & zeep::make_nvp("message", boodschap);
	}
};

struct Opname
{
	string	id;
	float	v;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long)
	{
		ar & zeep::make_nvp("id", id) & zeep::make_nvp("waarde", v);
	}
};


class my_rest_controller : public zh::rest_controller
{
  public:
	my_rest_controller()
		: zh::rest_controller("ajax")
	{
		map_post_request("opname", &my_rest_controller::post_opname, "opname");
		map_put_request("opname", &my_rest_controller::put_opname, "id", "opname");
		map_get_request("opname/{id}", &my_rest_controller::get_opname, "id");
		map_get_request("opname", &my_rest_controller::get_all_opnames);
		map_delete_request("opname", &my_rest_controller::delete_opname, "id");
	}

	// CRUD routines
	string post_opname(Opname opname)
	{
		string id = to_string(m_next_id++);
		m_opnames[id] = opname;
		return id;
	}

	void put_opname(string id, Opname opname)
	{
		if (m_opnames.count(id) == 0)
			throw std::runtime_error("Invalid ID for PUT");
		m_opnames[id] = opname;
	}

	Opname get_opname(std::string id)
	{
		if (m_opnames.count(id) == 0)
			throw std::runtime_error("Invalid ID for PUT");
		return m_opnames[id];
	}

	vector<Opname> get_all_opnames()
	{
		vector<Opname> result;
		transform(m_opnames.begin(), m_opnames.end(),
			back_inserter(result), [](auto e) { return e.second; });
		return result;
	}

	void delete_opname(string id)
	{
		if (m_opnames.count(id) == 0)
			throw std::runtime_error("Invalid ID for DELETE");
		m_opnames.erase(id);
	}

	// json put_opname(std::string id, Opname&& opname);
	// json delete_opname(std::string id);
  private:
	size_t m_next_id = 1;
	map<string,Opname> m_opnames;
};

int main()
{
	zh::webapp app("http://www.hekkelman.com/libzeep/ml", fs::current_path() / "docroot");
	app.add_controller(new my_rest_controller());

	app.bind("0.0.0.0", 10333);
    thread t(bind(&zh::webapp::run, &app, 2));
	t.join();

	return 0;
}