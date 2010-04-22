//  Copyright Maarten L. Hekkelman, Radboud University 2008.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "zeep/config.hpp"

#if SOAP_SERVER_HAS_PREFORK
#include "zeep/http/preforked-server.hpp"
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "zeep/server.hpp"

#include <iostream>

#include <boost/lexical_cast.hpp>

using namespace std;

int TRACE, VERBOSE;

// the data types used in our communication with the outside world
// are wrapped in a namespace.

namespace WSSearchNS
{

// the hit information

struct Hit
{
	string			db;
	string			id;
	string			title;
	float			score;

					Hit() : score(0) {}
	
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(db)
		   & BOOST_SERIALIZATION_NVP(id)
		   & BOOST_SERIALIZATION_NVP(title)
		   & BOOST_SERIALIZATION_NVP(score);
	}
};

// and the FindResult type.

struct FindResult
{
	int				count;
	vector<Hit>		hits;

					FindResult() : count(0) {}

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(count)
		   & BOOST_SERIALIZATION_NVP(hits);
	}
};

// these are the supported algorithms for ranked searching

enum Algorithm
{
	Vector,
	Dice,
	Jaccard
};

}

// now construct a server that can do several things:
// ListDatabanks:	simply return the list of searchable databanks
// Count:			a simple call taking two parameters and returning one 
// Find:			complex search routine taking several parameters and returning a complex type

class my_server : public zeep::server
{
  public:
						my_server(
							const string&				address,
							short						port,
							int							nr_of_threads,
							const string&				my_param);

	void				ListDatabanks(
							vector<string>&				databanks);

	void				Count(
							const string&				db,
							const string&				booleanquery,
							unsigned int&				result);

	void				Find(
							const string&				db,
							const vector<string>&		queryterms,
							WSSearchNS::Algorithm		algorithm,
							bool						alltermsrequired,
							const string&				booleanfilter,
							int							resultoffset,
							int							maxresultcount,
							WSSearchNS::FindResult&		out);

	void				ForceStop(string&				out);

  private:
	string				m_param;
};

my_server::my_server(const string& address, short port, int nr_of_threads, const string& param)
	: zeep::server("http://mrs.cmbi.ru.nl/mrsws/search", "zeep", address, port, nr_of_threads)
	, m_param(param)
{
	using namespace WSSearchNS;

	zeep::xml::serialize_struct<Hit>::set_struct_name("Hit");
	
	// The next call is needed since FindResult is defined in another namespace
	SOAP_XML_SET_STRUCT_NAME(FindResult);

	const char* kListDatabanksParameterNames[] = {
		"databank"
	};
	
	register_action("ListDatabanks", this, &my_server::ListDatabanks, kListDatabanksParameterNames);

	const char* kCountParameterNames[] = {
		"db", "booleanquery", "response"
	};
	
	register_action("Count", this, &my_server::Count, kCountParameterNames);
	
	// a new way of mapping enum values to strings. 
	zeep::xml::enum_map<Algorithm>::instance("Algorithm").add_enum()
		( "Vector", Vector )
		( "Dice", Dice )
		( "Jaccard", Jaccard )
		;

	// this is the old, macro based way of mapping the enums:
//	SOAP_XML_ADD_ENUM(Algorithm, Vector);
//	SOAP_XML_ADD_ENUM(Algorithm, Dice);
//	SOAP_XML_ADD_ENUM(Algorithm, Jaccard);

	const char* kFindParameterNames[] = {
		"db", "queryterms", "algorithm",
		"alltermsrequired", "booleanfilter", "resultoffset", "maxresultcount",
		"out"
	};
	
	register_action("Find", this, &my_server::Find, kFindParameterNames);

	const char* kForceStopParameterNames[] = {
		"out"
	};
	
	register_action("ForceStop", this, &my_server::ForceStop, kForceStopParameterNames);
}

void my_server::ListDatabanks(
	vector<string>&				response)
{
	response.push_back("sprot");
	response.push_back("trembl");
}

void my_server::Count(
	const string&				db,
	const string&				booleanquery,
	unsigned int&				result)
{
	if (db != "sprot" and db != "trembl" and db != "uniprot")
		throw zeep::exception("Unknown databank: %s", db.c_str());

	log() << db;

	result = 10;
}

void my_server::Find(
	const string&				db,
	const vector<string>&		queryterms,
	WSSearchNS::Algorithm		algorithm,
	bool						alltermsrequired,
	const string&				booleanfilter,
	int							resultoffset,
	int							maxresultcount,
	WSSearchNS::FindResult&		out)
{
	log() << db;

	// mock up some fake answer...
	out.count = 2;

	WSSearchNS::Hit h;
	h.db = "sprot";
	h.id = "104k_thepa";
	h.score = 1.0f;
	h.title = "bla bla bla";
	
	out.hits.push_back(h);

	h.db = "sprot";
	h.id = "108_lyces";
	h.score = 0.8f;
	h.title = "aap <&> noot mies";
	
	out.hits.push_back(h);

	h.db = "param";
	h.id = "param-id";
	h.score = 0.6f;
	h.title = m_param;
	
	out.hits.push_back(h);
}

void my_server::ForceStop(
	string&						out)
{
	exit(1);
}

int main(int argc, const char* argv[])
{
#if SOAP_SERVER_HAS_PREFORK
 	for (;;)
 	{
 		cout << "restarting server" << endl;
 		
	    sigset_t new_mask, old_mask;
	    sigfillset(&new_mask);
	    pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);
	
		zeep::http::preforked_server<my_server> server("0.0.0.0", 10333, 2, "bla bla");
		boost::thread t(
			boost::bind(&zeep::http::preforked_server<my_server>::run, &server));
	
	    pthread_sigmask(SIG_SETMASK, &old_mask, 0);
	
		// Wait for signal indicating time to shut down.
		sigset_t wait_mask;
		sigemptyset(&wait_mask);
		sigaddset(&wait_mask, SIGINT);
		sigaddset(&wait_mask, SIGQUIT);
		sigaddset(&wait_mask, SIGTERM);
		sigaddset(&wait_mask, SIGCHLD);
		pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
		int sig = 0;
		sigwait(&wait_mask, &sig);
	
		server.stop();
		t.join();
		
		if (sig == SIGCHLD)
		{
			int status, pid;
			pid = waitpid(-1, &status, WUNTRACED);
	
			if (pid != -1 and WIFSIGNALED(status))
				cout << "child " << pid << " terminated by signal " << WTERMSIG(status) << endl;
			continue;
		}
		
		break;
 	}
#elif defined(_MSC_VER)

	my_server server("0.0.0.0", 10333, 1, "blabla");
    boost::thread t(boost::bind(&my_server::run, &server));

#else
    sigset_t new_mask, old_mask;
    sigfillset(&new_mask);
    pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

	my_server server("0.0.0.0", 10333, 1, "blabla");
    boost::thread t(boost::bind(&my_server::run, &server));

    pthread_sigmask(SIG_SETMASK, &old_mask, 0);

	// Wait for signal indicating time to shut down.
	sigset_t wait_mask;
	sigemptyset(&wait_mask);
	sigaddset(&wait_mask, SIGINT);
	sigaddset(&wait_mask, SIGQUIT);
	sigaddset(&wait_mask, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
	int sig = 0;
	sigwait(&wait_mask, &sig);
	
	server.stop();
	t.join();
#endif
	return 0;
}
