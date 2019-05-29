// Copyright Maarten L. Hekkelman, Radboud University 2008-2019.
//        Copyright Maarten L. Hekkelman, 2014-2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <zeep/config.hpp>

#include <boost/algorithm/string.hpp>

#include <zeep/http/rest-controller.hpp>
#include <zeep/http/server.hpp>

namespace ba = boost::algorithm;

namespace zeep
{
namespace http
{

rest_controller::~rest_controller()
{
    for (auto mp: m_mountpoints)
        delete mp;
}

bool rest_controller::handle_request(const request& req, reply& rep)
{
	std::string p = req.uri;

	if (p.front() == '/')
		p.erase(0, 1);

	if (ba::starts_with(p, m_prefixPath))
		p.erase(0, m_prefixPath.length());
	
	if (p.front() == '/')
		p.erase(0, 1);

	auto pp = p.find('?');
	if (pp != std::string::npos)
		p.erase(pp);
	
	p = decode_url(p);

	auto mp = find_if(m_mountpoints.begin(), m_mountpoints.end(),
		[&](auto e) { return e->m_method == req.method and e->m_path == p; });
	
    bool result = false;

	if (mp != m_mountpoints.end())
    {
		(*mp)->call(req, rep);
        result = true;
    }

	return result;
}

}
}