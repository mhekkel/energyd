// Copyright Maarten L. Hekkelman, Radboud University 2008-2013.
//        Copyright Maarten L. Hekkelman, 2014-2019
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           http://www.boost.org/LICENSE_1_0.txt)

#ifndef SOAP_HTTP_REQUEST_HPP
#define SOAP_HTTP_REQUEST_HPP

#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <zeep/http/header.hpp>

namespace zeep
{
namespace http
{

enum class method_type
{
	UNDEFINED, OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE, CONNECT
};

inline constexpr const char* to_string(method_type method)
{
	switch (method)
	{
		case method_type::UNDEFINED:	return "UNDEFINED";
		case method_type::OPTIONS:		return "OPTIONS";
		case method_type::GET:			return "GET";
		case method_type::HEAD:			return "HEAD";
		case method_type::POST:			return "POST";
		case method_type::PUT:			return "PUT";
		case method_type::DELETE:		return "DELETE";
		case method_type::TRACE:		return "TRACE";
		case method_type::CONNECT:		return "CONNECT";
	}
}

/// request contains the parsed original HTTP request as received
/// by the server.

struct request
{
	using param = header;	// alias name

	method_type method;		///< POST, GET, etc.
	std::string uri;		///< The uri as requested
	int http_version_major; ///< HTTP major number (usually 1)
	int http_version_minor; ///< HTTP major number (0 or 1)
	std::vector<header>
		headers;		  ///< A list with zeep::http::header values
	std::string payload;  ///< For POST requests
	bool close;			  ///< Whether 'Connection: close' was specified
	std::string username; ///< The authenticated user for this request (filled in by webapp::validate_authentication)
	std::vector<param>
		path_params;      ///< The parameters found in the path (used by e.g. rest-controller)

	// for redirects...
	std::string local_address; ///< The address the request was received upon
	unsigned short local_port; ///< The port number the request was received upon

	boost::posix_time::ptime
	timestamp() const { return m_timestamp; }

	void clear(); ///< Reinitialises request and sets timestamp

	float accept(const char* type) const; ///< Return the value in the Accept header for type
	bool is_mobile() const;				  ///< Check HTTP_USER_AGENT to see if it is a mobile client
	bool keep_alive() const;			  ///< Check for Connection: keep-alive header

	std::string get_header(const char* name) const; ///< Return the named header
	void remove_header(const char* name);			///< Remove this header from the list of headers
	std::string get_request_line() const;			///< Return the (reconstructed) request line

	/// Fetch parameters from a request, either from the URL or from the payload in case
	/// the request contains a url-encoded or multi-part content-type header
	std::string get_parameter(const char* name) const; ///< Return the named parameter

	/// Return a parameter_map containing the cookies as found in the current request
	std::string get_cookie(const char* name) const;

	/// Can be used in code that sends HTTP requests
	void to_buffers(std::vector<boost::asio::const_buffer> &buffers);

	void debug(std::ostream& os) const;

  private:
	std::string m_request_line;
	boost::posix_time::ptime m_timestamp;
};

std::iostream& operator<<(std::iostream& io, request& req);

} // namespace http
} // namespace zeep

#endif
