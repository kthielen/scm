#ifndef STREAM_HTTPREADER_H_INCLUDED
#define STREAM_HTTPREADER_H_INCLUDED

#include <map>
#include <string>
#include <iostream>

namespace stream {

// parse an HTTP request
class http_request {
public:
    typedef std::map<std::string, std::string> named_values;

    http_request(std::istream& in);
    virtual ~http_request();

    const std::string& method() const;
    const std::string& url() const;

    unsigned int bytes_read() const;

    const named_values& headers() const;
    const named_values& cookies() const;
    const named_values& query_string_values() const;
    const named_values& post_values() const;
protected:
    unsigned int bytes_read_v;
    std::string  request_method_v;
    std::string  url_v;

    named_values headers_v, cookies_v, qsvalues_v, postvalues_v;

    http_request() { }
};

// parse an HTTP response
class http_response {
public:
    typedef std::map<std::string, std::string> named_values;

    http_response(std::istream& in);
    virtual ~http_response();

	const std::string& code() const;
	const std::string& state() const;

    unsigned int bytes_read() const;

    const named_values& headers() const;
	const std::string& body() const;
protected:
    unsigned int bytes_read_v;
	std::string  code_v;
	std::string  state_v;
	std::string  body_v;

    named_values headers_v;

    http_response() { }
};

}

#endif
