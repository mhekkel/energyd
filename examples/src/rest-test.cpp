//               Copyright Maarten L. Hekkelman.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <functional>

#include <zeep/http/webapp.hpp>
#include <zeep/http/md5.hpp>

#include <boost/filesystem.hpp>

using namespace std;
namespace zh = zeep::http;
namespace el = zeep::el;
namespace fs = boost::filesystem;

using json = el::element;

struct TellerStand
{
	int	tellerId;
	float stand;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long version)
	{
		ar & el::make_nvp("tellerId", tellerId)
		   & el::make_nvp("stand", stand);
	}
};

struct Opname
{
	string						id;
	boost::posix_time::ptime	date;
	vector<TellerStand> 		standen;

	template<typename Archive>
	void serialize(Archive& ar, unsigned long version)
	{
		ar & el::make_nvp("id", id)
		   & el::make_nvp("datum", datum)
		   & el::make_nvp("standen", standen);
	}
};

class my_first_rest_server : public zh::webapp
{
  public:

  private:

	// CRUD routines
	json post_opname(Opname&& opname);
	json get_opname(std::string id);
	json put_opname(std::string id, Opname&& opname);
	json delete_opname(std::string id);

};