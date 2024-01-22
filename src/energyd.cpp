//               Copyright Maarten L. Hekkelman.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#define WEBAPP_USES_RESOURCES 1

#include "mrsrc.hpp"
#include "revision.hpp"

#include "data-service.hpp"
#include "p1-service.hpp"
#include "sessy-service.hpp"

#include <utility>

#include <zeep/http/daemon.hpp>
#include <zeep/http/html-controller.hpp>
#include <zeep/http/server.hpp>

#include <zeep/http/error-handler.hpp>
#include <zeep/http/rest-controller.hpp>
#include <zeep/json/parser.hpp>

#include <mcfp.hpp>

#include <pqxx/pqxx>

#include <functional>
#include <iostream>
#include <tuple>

namespace fs = std::filesystem;

// --------------------------------------------------------------------

std::chrono::system_clock::time_point makeTimePoint(const std::string &s)
{
	using namespace std::literals;
	using namespace date;
	using namespace std::chrono;

	std::chrono::system_clock::time_point result;

	std::istringstream is(s);
	is >> parse("%F %T", result);

	if (is.bad() or is.fail())
		throw std::runtime_error("invalid formatted date");

	return result;
}

struct Opname
{
	std::string id;
	std::chrono::system_clock::time_point datum;
	std::map<std::string, float> standen;

	template <typename Archive>
	void serialize(Archive &ar, unsigned long version)
	{
		ar & zeep::make_nvp("id", id)
		   & zeep::make_nvp("datum", datum)
		   & zeep::make_nvp("standen", standen);
	}
};

struct Teller
{
	std::string id;
	std::string naam;
	std::string naam_kort;
	int schaal;

	template <typename Archive>
	void serialize(Archive &ar, unsigned long)
	{
		ar & zeep::make_nvp("id", id)
		   & zeep::make_nvp("naam", naam)
		   & zeep::make_nvp("korteNaam", naam_kort)
		   & zeep::make_nvp("schaal", schaal);
	}
};

enum class aggregatie_type
{
	dag,
	week,
	maand,
	jaar
};

void to_element(zeep::json::element &e, aggregatie_type aggregatie)
{
	switch (aggregatie)
	{
		case aggregatie_type::dag: e = "dag"; break;
		case aggregatie_type::week: e = "week"; break;
		case aggregatie_type::maand: e = "maand"; break;
		case aggregatie_type::jaar: e = "jaar"; break;
	}
}

void from_element(const zeep::json::element &e, aggregatie_type &aggregatie)
{
	if (e == "dag")
		aggregatie = aggregatie_type::dag;
	else if (e == "week")
		aggregatie = aggregatie_type::week;
	else if (e == "maand")
		aggregatie = aggregatie_type::maand;
	else if (e == "jaar")
		aggregatie = aggregatie_type::jaar;
	else
		throw std::runtime_error("Ongeldige aggregatie");
}

enum class grafiek_type
{
	warmte,
	electriciteit,
	electriciteit_hoog,
	electriciteit_laag,
	electriciteit_verbruik,
	electriciteit_levering,
	electriciteit_verbruik_hoog,
	electriciteit_verbruik_laag,
	electriciteit_levering_hoog,
	electriciteit_levering_laag
};

void to_element(zeep::json::element &e, grafiek_type type)
{
	switch (type)
	{
		case grafiek_type::warmte: e = "warmte"; break;
		case grafiek_type::electriciteit: e = "electriciteit"; break;
		case grafiek_type::electriciteit_hoog: e = "electriciteit-hoog"; break;
		case grafiek_type::electriciteit_laag: e = "electriciteit-laag"; break;
		case grafiek_type::electriciteit_verbruik: e = "electriciteit-verbruik"; break;
		case grafiek_type::electriciteit_levering: e = "electriciteit-levering"; break;
		case grafiek_type::electriciteit_verbruik_hoog: e = "electriciteit-verbruik-hoog"; break;
		case grafiek_type::electriciteit_verbruik_laag: e = "electriciteit-verbruik-laag"; break;
		case grafiek_type::electriciteit_levering_hoog: e = "electriciteit-levering-hoog"; break;
		case grafiek_type::electriciteit_levering_laag: e = "electriciteit-levering-laag"; break;
	}
}

void from_element(const zeep::json::element &e, grafiek_type &type)
{
	if (e == "warmte")
		type = grafiek_type::warmte;
	else if (e == "electriciteit")
		type = grafiek_type::electriciteit;
	else if (e == "electriciteit-hoog")
		type = grafiek_type::electriciteit_hoog;
	else if (e == "electriciteit-laag")
		type = grafiek_type::electriciteit_laag;
	else if (e == "electriciteit-verbruik")
		type = grafiek_type::electriciteit_verbruik;
	else if (e == "electriciteit-levering")
		type = grafiek_type::electriciteit_levering;
	else if (e == "electriciteit-verbruik-hoog")
		type = grafiek_type::electriciteit_verbruik_hoog;
	else if (e == "electriciteit-verbruik-laag")
		type = grafiek_type::electriciteit_verbruik_laag;
	else if (e == "electriciteit-levering-hoog")
		type = grafiek_type::electriciteit_levering_hoog;
	else if (e == "electriciteit-levering-laag")
		type = grafiek_type::electriciteit_levering_laag;
	else
		throw std::runtime_error("Ongeldige grafiek type");
}

std::string selector(grafiek_type g)
{
	switch (g)
	{
		case grafiek_type::warmte:
			return "SELECT a.tijd, SUM(c.teken * b.stand) "
				   " FROM opname a LEFT OUTER JOIN tellerstand b LEFT OUTER JOIN teller c ON b.teller_id = c.id ON a.id = b.opname_id "
				   " WHERE c.id IN (1) GROUP BY a.tijd ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit:
			return "SELECT a.tijd, SUM(c.teken * b.stand) "
				   " FROM opname a LEFT OUTER JOIN tellerstand b LEFT OUTER JOIN teller c ON b.teller_id = c.id ON a.id = b.opname_id "
				   " WHERE c.id IN (2, 3, 4, 5) GROUP BY a.tijd ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_hoog:
			return "SELECT a.tijd, SUM(c.teken * b.stand) "
				   " FROM opname a LEFT OUTER JOIN tellerstand b LEFT OUTER JOIN teller c ON b.teller_id = c.id ON a.id = b.opname_id "
				   " WHERE c.id IN (3, 5) GROUP BY a.tijd ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_laag:
			return "SELECT a.tijd, SUM(c.teken * b.stand) "
				   " FROM opname a LEFT OUTER JOIN tellerstand b LEFT OUTER JOIN teller c ON b.teller_id = c.id ON a.id = b.opname_id "
				   " WHERE c.id IN (2, 4) GROUP BY a.tijd ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_verbruik:
			return "SELECT a.tijd, SUM(b.stand) "
				   " FROM opname a LEFT OUTER JOIN tellerstand b ON a.id = b.opname_id "
				   " WHERE b.teller_id IN (2, 3) GROUP BY a.tijd ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_levering:
			return "SELECT a.tijd, SUM(b.stand) "
				   " FROM opname a LEFT OUTER JOIN tellerstand b ON a.id = b.opname_id "
				   " WHERE b.teller_id IN (4, 5) GROUP BY a.tijd ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_verbruik_hoog:
			return "SELECT a.tijd, b.stand "
				   " FROM opname a LEFT OUTER JOIN tellerstand b ON a.id = b.opname_id "
				   " WHERE b.teller_id = 3 ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_verbruik_laag:
			return "SELECT a.tijd, b.stand "
				   " FROM opname a LEFT OUTER JOIN tellerstand b ON a.id = b.opname_id "
				   " WHERE b.teller_id = 2 ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_levering_hoog:
			return "SELECT a.tijd, b.stand "
				   " FROM opname a LEFT OUTER JOIN tellerstand b ON a.id = b.opname_id "
				   " WHERE b.teller_id = 5 ORDER BY a.tijd ASC";
		case grafiek_type::electriciteit_levering_laag:
			return "SELECT a.tijd, b.stand "
				   " FROM opname a LEFT OUTER JOIN tellerstand b ON a.id = b.opname_id "
				   " WHERE b.teller_id = 4 ORDER BY a.tijd ASC";
		default:
			return "";
	}
}

struct DataPunt
{
	std::string date;
	float v;
	float a;
	float sd;
	float ma;

	template <typename Archive>
	void serialize(Archive &ar, unsigned long)
	{
		ar & zeep::make_nvp("d", date)
		   & zeep::make_nvp("v", v)
		   & zeep::make_nvp("a", a)
		   & zeep::make_nvp("sd", sd)
		   & zeep::make_nvp("ma", ma);
	}
};

// --------------------------------------------------------------------

using StandMap = std::map<std::chrono::system_clock::time_point, float>;

class DataService
{
  public:
	static void init(const std::string &dbConnectString);
	static DataService &instance();

	std::string post_opname(Opname opname)
	{
		pqxx::work tx(get_connection());
		auto r = tx.exec_prepared1("insert-opname");

		int opnameId = r[0].as<int>();

		for (auto stand : opname.standen)
			tx.exec_prepared("insert-stand", opnameId, stol(stand.first), stand.second);

		tx.commit();

		return std::to_string(opnameId);
	}

	void put_opname(std::string opnameId, Opname opname)
	{
		pqxx::work tx(get_connection());

		for (auto stand : opname.standen)
			tx.exec_prepared("update-stand", stand.second, opnameId, stol(stand.first));

		tx.commit();
	}

	Opname get_opname(std::string id)
	{
		pqxx::work tx(get_connection());
		auto rows = tx.exec_prepared("get-opname", id);

		if (rows.empty())
			throw std::runtime_error("opname niet gevonden");

		Opname result{ rows.front()[0].as<std::string>(), makeTimePoint(rows.front()[1].as<std::string>()) };

		for (auto row : rows)
			result.standen[row[2].as<std::string>()] = row[3].as<float>();

		return result;
	}

	Opname get_last_opname()
	{
		pqxx::work tx(get_connection());
		auto rows = tx.exec_prepared("get-last-opname");

		if (rows.empty())
			throw std::runtime_error("opname niet gevonden");

		Opname result{ rows.front()[0].as<std::string>(), makeTimePoint(rows.front()[1].as<std::string>()) };

		for (auto row : rows)
			result.standen[row[2].as<std::string>()] = row[3].as<float>();

		return result;
	}

	std::vector<Opname> get_all_opnames()
	{
		std::vector<Opname> result;

		pqxx::work tx(get_connection());

		auto rows = tx.exec_prepared("get-opname-all");
		for (auto row : rows)
		{
			auto id = row[0].as<std::string>();

			if (result.empty() or result.back().id != id)
				result.push_back({ id, makeTimePoint(row[1].as<std::string>()) });

			result.back().standen[row[2].as<std::string>()] = row[3].as<float>();
		}

		return result;
	}

	void delete_opname(std::string id)
	{
		pqxx::work tx(get_connection());
		tx.exec_prepared("del-opname", id);
		tx.commit();
	}

	std::vector<Teller> get_tellers()
	{
		std::vector<Teller> result;

		pqxx::work tx(get_connection());

		auto rows = tx.exec_prepared("get-tellers-all");
		for (auto row : rows)
		{
			auto c1 = row.column_number("id");
			auto c2 = row.column_number("naam");
			auto c3 = row.column_number("naam_kort");
			auto c4 = row.column_number("schaal");

			result.push_back({ row[c1].as<std::string>(), row[c2].as<std::string>(), row[c3].as<std::string>(), row[c4].as<int>() });
		}

		return result;
	}

	StandMap get_stand_map(grafiek_type type)
	{
		pqxx::work tx(get_connection());

		StandMap sm;

		for (auto r : tx.exec(selector(type)))
			sm[makeTimePoint(r[0].as<std::string>())] = r[1].as<float>();

		return sm;
	}

	void reset()
	{
		sConnection.reset();
	}

  private:
	DataService(const std::string &dbConnectString);

	pqxx::connection &get_connection();

	std::string mConnectString;

	static std::unique_ptr<DataService> sInstance;
	static thread_local std::unique_ptr<pqxx::connection> sConnection;
};

std::unique_ptr<DataService> DataService::sInstance;
thread_local std::unique_ptr<pqxx::connection> DataService::sConnection;

void DataService::init(const std::string &dbConnectString)
{
	sInstance.reset(new DataService(dbConnectString));
}

DataService &DataService::instance()
{
	return *sInstance;
}

pqxx::connection &DataService::get_connection()
{
	if (not sConnection)
	{
		sConnection.reset(new pqxx::connection(mConnectString));

		sConnection->prepare("get-opname-all",
			"SELECT a.id AS id, a.tijd AS tijd, b.teller_id AS teller_id, b.stand AS stand"
			" FROM opname a, tellerstand b"
			" WHERE a.id = b.opname_id"
			" ORDER BY a.tijd DESC");

		sConnection->prepare("get-opname",
			"SELECT a.id AS id, a.tijd AS tijd, b.teller_id AS teller_id, b.stand AS stand"
			" FROM opname a, tellerstand b"
			" WHERE a.id = b.opname_id AND a.id = $1");

		sConnection->prepare("get-last-opname",
			"SELECT a.id AS id, a.tijd AS tijd, b.teller_id AS teller_id, b.stand AS stand"
			" FROM opname a, tellerstand b"
			" WHERE a.id = b.opname_id AND a.id = (SELECT MAX(id) FROM opname)");

		sConnection->prepare("insert-opname", "INSERT INTO opname DEFAULT VALUES RETURNING id");
		sConnection->prepare("insert-stand", "INSERT INTO tellerstand (opname_id, teller_id, stand) VALUES($1, $2, $3);");

		sConnection->prepare("update-stand", "UPDATE tellerstand SET stand = $1 WHERE opname_Id = $2 AND teller_id = $3;");

		sConnection->prepare("del-opname", "DELETE FROM opname WHERE id=$1");

		sConnection->prepare("get-tellers-all",
			"SELECT id, naam, naam_kort, schaal FROM teller ORDER BY id");
	}
	return *sConnection;
}

DataService::DataService(const std::string &dbConnectString)
	: mConnectString(dbConnectString)
{
}

// --------------------------------------------------------------------

class e_rest_controller : public zeep::http::rest_controller
{
  public:
	e_rest_controller()
		: zeep::http::rest_controller("ajax")
	{
		map_post_request("opname", &e_rest_controller::post_opname, "opname");
		map_put_request("opname/{id}", &e_rest_controller::put_opname, "id", "opname");
		map_get_request("opname/{id}", &e_rest_controller::get_opname, "id");
		map_get_request("opname", &e_rest_controller::get_all_opnames);
		map_delete_request("opname/{id}", &e_rest_controller::delete_opname, "id");

		map_get_request("data/{type}/{aggr}", &e_rest_controller::get_grafiek, "type", "aggr");
	}

	// CRUD routines
	std::string post_opname(Opname opname)
	{
		return DataService::instance().post_opname(opname);
	}

	void put_opname(std::string opnameId, Opname opname)
	{
		DataService::instance().put_opname(opnameId, opname);
	}

	Opname get_opname(std::string id)
	{
		return DataService::instance().get_opname(id);
	}

	Opname get_last_opname()
	{
		return DataService::instance().get_last_opname();
	}

	std::vector<Opname> get_all_opnames()
	{
		return DataService::instance().get_all_opnames();
	}

	void delete_opname(std::string id)
	{
		DataService::instance().delete_opname(id);
	}

	std::vector<Teller> get_tellers()
	{
		return DataService::instance().get_tellers();
	}

	// GrafiekData get_grafiek(const string& type, aggregatie_type aggregatie);
	std::vector<DataPunt> get_grafiek(grafiek_type type, aggregatie_type aggregatie);
};

// --------------------------------------------------------------------

float interpolateStand(const StandMap &data, std::chrono::system_clock::time_point t)
{
	float result = 0;

	for (;;)
	{
		if (data.empty())
			break;

		auto ub = data.upper_bound(t);
		if (ub == data.end())
		{
			result = prev(ub)->second;
			break;
		}

		if (ub == data.begin())
		{
			result = ub->second;
			break;
		}

		auto b = prev(ub);
		assert(b->first <= t);

		double d1 = (ub->first - b->first).count();
		double d2 = (t - b->first).count();

		auto df = ub->second - b->second;

		result = b->second + df * (d2 / d1);

		break;
	}

	return result;
}

float interpoleerVerbruik(const StandMap &data, std::chrono::system_clock::time_point t, aggregatie_type aggr)
{
	using namespace date;
	using namespace std::chrono;
	using namespace std::literals;

	auto t1 = t, t2 = t;

	if (t1 > prev(data.end())->first)
		t1 = prev(data.end())->first;

	switch (aggr)
	{
		case aggregatie_type::dag:
			t2 -= date::days{ 1 };
			break;
		case aggregatie_type::week:
			t2 -= date::weeks{ 1 };
			break;
		case aggregatie_type::maand:
			t2 -= date::months{ 1 };
			break;
		case aggregatie_type::jaar:
			t2 -= date::years{ 1 };
			break;
	}

	if (t2 > t1)
		t2 = t1;

	float result = 0;

	if (t1 > t2)
	{
		float s1 = interpolateStand(data, t1);
		float s2 = interpolateStand(data, t2);
		result = (24 * 60 * 60) * (s1 - s2) / floor<std::chrono::seconds>(t1 - t2).count();
	}

	return result;
}

std::vector<float> waarden_op_deze_dag(StandMap &sm, std::chrono::system_clock::time_point t, aggregatie_type aggr)
{
	using namespace date;
	// using namespace std::chrono;
	using namespace std::literals;

	auto smb = sm.begin()->first;

	std::vector<float> v;

	while (t >= smb)
	{
		v.push_back(interpoleerVerbruik(sm, t, aggr));
		t -= years{ 1 };
	}

	return v;
}

std::vector<DataPunt> e_rest_controller::get_grafiek(grafiek_type type, aggregatie_type aggr)
{
	using namespace date;
	// using namespace std::chrono;
	using namespace std::literals;

	StandMap sm = DataService::instance().get_stand_map(type);

	auto nu = std::chrono::system_clock::now();

	auto jaar = year_month_day{ floor<days>(nu) }.year();

	auto b = sys_days{ year{ jaar } / January / 1 } + 0h;
	auto e = sys_days{ year{ jaar } / December / 31 } + 0h;

	std::vector<DataPunt> result;

	for (auto d = b; d <= e; d += 24h)
	{
		auto v = waarden_op_deze_dag(sm, d + days{1}, aggr);

		auto N = v.size();

		DataPunt pt{};

		pt.date = date::format("%F", d);

		if (d > nu + days{ 2 })
		{
			pt.v = v[1];
			pt.ma = interpoleerVerbruik(sm, d - years{ 1 }, aggregatie_type::jaar);
		}
		else if (d <= nu + days{ 1 })
		{
			pt.v = v.front();
			pt.ma = interpoleerVerbruik(sm, d, aggregatie_type::jaar);
		}

		if (N > 1)
		{
			float sum = 0, sum2 = 0;
			for (size_t i = 1; i < N; ++i)
				sum += v[i];

			float avg = sum / (N - 1);
			pt.a = avg;

			for (size_t i = 1; i < N; ++i)
				sum2 += (v[i] - avg) * (v[i] - avg);

			pt.sd = std::sqrt(sum2) / N;
		}

		result.emplace_back(std::move(pt));
	}

	return result;
}

// --------------------------------------------------------------------

class e_web_controller : public zeep::http::html_controller
{
  public:
	e_web_controller()
	{
		map_get("", &e_web_controller::opname);
		map_get("opnames", &e_web_controller::opname);
		mount("invoer", &e_web_controller::invoer);
		mount("grafiek", &e_web_controller::grafiek);

		mount("{css,scripts,fonts}/", &e_web_controller::handle_file);
	}

	zeep::http::reply opname(const zeep::http::scope &scope);
	void invoer(const zeep::http::request &request, const zeep::http::scope &scope, zeep::http::reply &reply);
	void grafiek(const zeep::http::request &request, const zeep::http::scope &scope, zeep::http::reply &reply);
};

zeep::http::reply e_web_controller::opname(const zeep::http::scope &scope)
{
	zeep::http::scope sub(scope);

	sub.put("page", "opname");

	auto v = DataService::instance().get_all_opnames();
	zeep::json::element opnames;
	to_element(opnames, v);
	sub.put("opnames", opnames);

	auto u = DataService::instance().get_tellers();
	zeep::json::element tellers;
	to_element(tellers, u);
	sub.put("tellers", tellers);

	// huidige stand

	auto huidig = DataService::instance().get_last_opname();
	huidig.id.clear();
	huidig.datum = {};

	auto p1_w = P1Service::instance().get_current();

	huidig.standen["2"] = p1_w.verbruik_laag;
	huidig.standen["3"] = p1_w.verbruik_hoog;
	huidig.standen["4"] = p1_w.levering_laag;
	huidig.standen["5"] = p1_w.levering_hoog;

	zeep::json::element opname;
	to_element(opname, huidig);
	sub.put("huidig", opname);

	return get_template_processor().create_reply_from_template("opnames", sub);
}

void e_web_controller::invoer(const zeep::http::request &request, const zeep::http::scope &scope, zeep::http::reply &reply)
{
	zeep::http::scope sub(scope);

	sub.put("page", "invoer");

	Opname o;

	std::string id = request.get_parameter("id", "");
	if (id.empty())
	{
		o = DataService::instance().get_last_opname();
		o.id.clear();
		o.datum = {};

		auto p1_w = P1Service::instance().get_current();

		o.standen["2"] = p1_w.verbruik_laag;
		o.standen["3"] = p1_w.verbruik_hoog;
		o.standen["4"] = p1_w.levering_laag;
		o.standen["5"] = p1_w.levering_hoog;
	}
	else
		o = DataService::instance().get_opname(id);

	zeep::json::element opname;
	to_element(opname, o);
	sub.put("opname", opname);

	auto u = DataService::instance().get_tellers();
	zeep::json::element tellers;
	to_element(tellers, u);
	sub.put("tellers", tellers);

	get_template_processor().create_reply_from_template("invoer.html", sub, reply);
}

void e_web_controller::grafiek(const zeep::http::request &request, const zeep::http::scope &scope, zeep::http::reply &reply)
{
	zeep::http::scope sub(scope);

	sub.put("page", "grafiek");

	auto v = DataService::instance().get_all_opnames();
	zeep::json::element opnames;
	to_element(opnames, v);
	sub.put("opnames", opnames);

	auto u = DataService::instance().get_tellers();
	zeep::json::element tellers;
	to_element(tellers, u);
	sub.put("tellers", tellers);

	get_template_processor().create_reply_from_template("grafiek.html", sub, reply);
}

// --------------------------------------------------------------------

class e_error_handler : public zeep::http::error_handler
{
  public:
	virtual bool create_error_reply(const zeep::http::request &req, std::exception_ptr eptr, zeep::http::reply &reply)
	{
		try
		{
			std::rethrow_exception(eptr);
		}
		catch (pqxx::broken_connection &ex)
		{
			std::cerr << ex.what() << std::endl;
			DataService::instance().reset();
		}
		catch (...)
		{
		}

		return false;
	}
};

// --------------------------------------------------------------------

int main(int argc, const char *argv[])
{
	int result = 0;

	auto &config = mcfp::config::instance();

	config.init("energyd [options] command",
		mcfp::make_option("help,h", "Display help message"),
		mcfp::make_option("verbose,v", "Verbose output"),
		mcfp::make_option("version", "Show version information"),

		mcfp::make_option<std::string>("address", "0.0.0.0", "External address"),
		mcfp::make_option<uint16_t>("port", 10336, "Port to listen to"),
		mcfp::make_option("no-daemon,F", "Do not fork into background"),
		mcfp::make_option<std::string>("user,u", "www-data", "User to run the daemon"),

		mcfp::make_option<std::string>("databank", "The Postgresql connection string"),

		mcfp::make_option<std::string>("p1-device", "/dev/ttyUSB0", "The name of the device used to communicate with the P1 port"),
		
		mcfp::make_option<std::string>("sessy-1", "URL to fetch the status of sessy number 1"),
		mcfp::make_option<std::string>("sessy-2", "URL to fetch the status of sessy number 2"),
		mcfp::make_option<std::string>("sessy-3", "URL to fetch the status of sessy number 3"),
		mcfp::make_option<std::string>("sessy-4", "URL to fetch the status of sessy number 4"),
		mcfp::make_option<std::string>("sessy-5", "URL to fetch the status of sessy number 5"),
		mcfp::make_option<std::string>("sessy-6", "URL to fetch the status of sessy number 6")
		);

	std::error_code ec;
	config.parse(argc, argv, ec);
	if (ec)
	{
		std::cerr << "Error parsing arguments: " << ec.message() << std::endl;
		return 1;
	}

	if (config.has("version"))
	{
		write_version_string(std::cout, config.has("verbose"));
		return 0;
	}

	if (config.operands().size() != 1 or config.has("help"))
	{
		std::cout << config << std::endl
				  << R"(
Command should be either:

    start     start a new server
    stop      stop a running server
    status    get the status of a running server
    reload    restart a running server with new options
				)" << std::endl;

		return config.has("help") ? 0 : 1;
	}

	config.parse_config_file("config", "energyd.conf", { ".", "/etc" }, ec);
	if (ec)
	{
		std::cerr << "Error parsing config file: " << ec.message() << std::endl;
		return 1;
	}

	DataService::init(config.get("databank"));
	DataService_v2::instance();

	zeep::http::daemon server([]()
		{
			auto s = new zeep::http::server("docroot");

			P1Service::init(s->get_io_context());
			SessyService::init(s->get_io_context());

#ifndef NDEBUG
			s->set_template_processor(new zeep::http::file_based_html_template_processor("docroot"));
#else
			s->set_template_processor(new zeep::http::rsrc_based_html_template_processor());
#endif

			s->add_controller(new e_rest_controller());
			s->add_controller(new e_web_controller());
			s->add_error_handler(new e_error_handler());

			return s; },
		"energyd");

	std::string command = config.operands().front();

	if (command == "start")
	{
		std::string address = config.get("address");
		uint16_t port = config.get<uint16_t>("port");

		if (address.find(':') != std::string::npos)
			std::cout << "starting server at http://[" << address << "]:" << port << '/' << std::endl;
		else
			std::cout << "starting server at http://" << address << ':' << port << '/' << std::endl;

		if (config.has("no-daemon"))
			result = server.run_foreground(address, port);
		else
		{
			std::string user = config.get("user");
			result = server.start(address, port, 1, 2, user);
		}
	}
	else if (command == "stop")
		result = server.stop();
	else if (command == "status")
		result = server.status();
	else if (command == "reload")
		result = server.reload();
	else
	{
		std::cerr << "Invalid command" << std::endl;
		result = 1;
	}

	return result;
}