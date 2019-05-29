//               Copyright Maarten L. Hekkelman.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <functional>
#include <tuple>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

#include <zeep/http/webapp.hpp>
#include <zeep/http/md5.hpp>

#include <boost/filesystem.hpp>
#include <zeep/el/parser.hpp>
#include <zeep/http/rest-controller.hpp>

#include <pqxx/pqxx>

using namespace std;
namespace zh = zeep::http;
namespace el = zeep::el;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;
namespace po = boost::program_options;

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
	string	tijd;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long)
	{
		ar & zeep::make_nvp("id", id) & zeep::make_nvp("tijd", tijd);
	}
};


class my_rest_controller : public zh::rest_controller
{
  public:
	my_rest_controller(const string& connectionString)
		: zh::rest_controller("ajax")
		, m_connection(connectionString)
	{
		map_post_request("opname", &my_rest_controller::post_opname, "opname");
		map_put_request("opname", &my_rest_controller::put_opname, "id", "opname");
		map_get_request("opname/{id}", &my_rest_controller::get_opname, "id");
		map_get_request("opname", &my_rest_controller::get_all_opnames);
		map_delete_request("opname", &my_rest_controller::delete_opname, "id");

		m_connection.prepare("get-opname-all", "SELECT * FROM opname");
		m_connection.prepare("get-opname", "SELECT * FROM opname WHERE id=$1");
		m_connection.prepare("put-opname", "INSERT INTO opname (id, tijd) VALUES($1, $2)");
		m_connection.prepare("del-opname", "DELETE FROM opname WHERE id=$1");
	}

	// CRUD routines
	string post_opname(Opname opname)
	{
		// string id = to_string(m_next_id++);
		// m_opnames[id] = opname;
		// return id;
		return "";
	}

	void put_opname(string id, Opname opname)
	{
		// if (m_opnames.count(id) == 0)
		// 	throw std::runtime_error("Invalid ID for PUT");
		// m_opnames[id] = opname;
	}

	Opname get_opname(std::string id)
	{
		pqxx::transaction tx(m_connection);
		auto rows = tx.prepared("get-opname")(id).exec();

		if (rows.size() != 1)
			throw runtime_error("Unexpected number of opnames");

		auto row = rows.front();
		auto c1 = row.column_number("id");
		auto c2 = row.column_number("tijd");

		return {row[c1].as<string>(), row[c2].as<string>()};
	}

	vector<Opname> get_all_opnames()
	{
		vector<Opname> result;

		pqxx::transaction tx(m_connection);

		auto rows = tx.prepared("get-opname-all").exec();
		for (auto row: rows)
		{
			auto c1 = row.column_number("id");
			auto c2 = row.column_number("tijd");

			result.push_back({row[c1].as<string>(), row[c2].as<string>()});
		}

		return result;
	}

	void delete_opname(string id)
	{
		pqxx::transaction tx(m_connection);
		tx.prepared("del-opname")(id).exec();
		tx.commit();
	}

  private:
	pqxx::connection m_connection;
};

int main(int argc, const char* argv[])
{
	po::options_description visible_options(argv[0] + " options"s);
	visible_options.add_options()
		("help,h",										"Display help message")
		// ("version",										"Print version")
		("verbose,v",									"Verbose output")
		
		("address",				po::value<string>(),	"External address, default is 0.0.0.0")
		("port",				po::value<uint16_t>(),	"Port to listen to, default is 10336")
		// ("no-daemon,F",									"Do not fork into background")
		// ("user,u",				po::value<string>(),	"User to run the daemon")
		// ("logfile",				po::value<string>(),	"Logfile to write to, default /var/log/rama-angles.log")

		("db-host",				po::value<string>(),	"Database host")
		("db-port",				po::value<string>(),	"Database port")
		("db-dbname",			po::value<string>(),	"Database name")
		("db-user",				po::value<string>(),	"Database user name")
		("db-password",			po::value<string>(),	"Database password")
		;
	
	po::options_description hidden_options("hidden options");
	hidden_options.add_options()
		("debug,d",				po::value<int>(),		"Debug level (for even more verbose output)");

	po::options_description cmdline_options;
	cmdline_options.add(visible_options).add(hidden_options);

	po::positional_options_description p;
	// p.add("xyzin", 1);
	// p.add("output", 1);
	
	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
	po::notify(vm);

	// --------------------------------------------------------------------

	// if (vm.count("version"))
	// {
	// 	cout << argv[0] << " version " << VERSION << endl;
	// 	exit(0);
	// }

	if (vm.count("help"))
	{
		cerr << visible_options << endl;
		exit(0);
	}
	
	try
	{
		vector<string> vConn;
		for (string opt: { "db-host", "db-port", "db-dbname", "db-user", "db-password" })
		{
			if (vm.count(opt) == 0)
				continue;
			
			vConn.push_back(opt.substr(3) + "=" + vm[opt].as<string>());
		}

		zh::webapp app("http://www.hekkelman.com/libzeep/ml", fs::current_path() / "docroot");
		app.add_controller(new my_rest_controller(ba::join(vConn, " ")));

		string address = "0.0.0.0";
		uint16_t port = 10333;
		if (vm.count("address"))
			address = vm["address"].as<string>();
		if (vm.count("port"))
			port = vm["port"].as<uint16_t>();

		app.bind(address, port);
		thread t(bind(&zh::webapp::run, &app, 2));
		t.join();

	}
	catch (const exception& ex)
	{
		cerr << ex.what() << endl;
		exit(1);
	}



	return 0;
}