#include <scm/stream/http_reader.hpp>
#include <scm/parse/StreamFSM.hpp>
#include <scm/str/Util.hpp>
#include <stdexcept>

using namespace str;
using namespace parse;

namespace stream {

// read an HTTP request
enum HTTPReadState {
    ReadingRequestMethod,
    ReadingRequestURL,
    ReadingHTTPVersion,
    ReadingHeader,
    ReadingPostData
};

class HTTPRequestReader;
typedef StreamFSM<char, HTTPReadState, HTTPRequestReader> HTTPRequestReaderMachine;

class HTTPRequestReader : public http_request, public HTTPRequestReaderMachine {
public:
    HTTPRequestReader() : HTTPRequestReaderMachine(ReadingRequestMethod), found_non_ws(false) {
		// 'bytes read' comes from HTTPRequestData - it's supposed to indicate the total number of
		// read in processing the request
		this->bytes_read_v = 0;
		
		AddStateHandler(ReadingRequestMethod, &HTTPRequestReader::ReadRequestMethod);
		AddStateHandler(ReadingRequestURL,    &HTTPRequestReader::ReadRequestURL);
		AddStateHandler(ReadingHTTPVersion,   &HTTPRequestReader::ReadHTTPVersion);
		AddStateHandler(ReadingHeader,        &HTTPRequestReader::ReadHeader);
		AddStateHandler(ReadingPostData,      &HTTPRequestReader::ReadPostData);
		
		AddTransitionHandler(AnyState,		   AnyState,	     &HTTPRequestReader::CountBytesReceived);
		AddTransitionHandler(ReadingRequestMethod, AnyState,         &HTTPRequestReader::ProcessRequestMethod);
		AddTransitionHandler(ReadingRequestURL,    AnyState,         &HTTPRequestReader::ProcessURL);
		AddTransitionHandler(AnyState,             ReadingPostData,  &HTTPRequestReader::PrepForPost);
		AddTransitionHandler(ReadingHeader,        ReadingHeader,    &HTTPRequestReader::ProcessHeader);
    }
private:
    unsigned int post_len_v;
    unsigned int post_bytes_read_v;
    bool         found_non_ws;

    void CountBytesReceived(elem_iter start, elem_iter end) {
		this->bytes_read_v += end - start;
    }

    void ReadRequestMethod(const char& c) {
		if (!is_whitespace(c) && !this->found_non_ws) {
		    this->found_non_ws = true;
		} else if (this->found_non_ws && is_whitespace(c)) {
			this->found_non_ws = false;
			SetState(ReadingRequestURL);
		}
    }

    void ReadRequestURL(const char& c) {
		if (!is_whitespace(c) && !this->found_non_ws) {
			this->found_non_ws = true;
		} else if (this->found_non_ws && is_whitespace(c)) {
			this->found_non_ws = false;
			SetState(ReadingHTTPVersion);
		}
    }

    void ReadHTTPVersion(const char& c) {
		if (!is_whitespace(c) && !this->found_non_ws) {
			this->found_non_ws = true;
		}
	
		if (c == '\n') {
			this->found_non_ws = false;
			SetState(ReadingHeader);
		}
    }

    void ReadHeader(const char& c) {
		if (!is_whitespace(c) && !this->found_non_ws) {
			this->found_non_ws = true;
		}

		if ( (c == '\n') && !this->found_non_ws ) {
			this->found_non_ws = false;

			// we've read the last header, read POST data if applicable, or detach
			if (this->request_method_v != "post")
				Detach();
			else
				SetState(ReadingPostData);
		} else if (c == '\n') {
			// we're at the end of a header, move to the next one
			this->found_non_ws = false;
			SetState(ReadingHeader);
		}
    }

    void PrepForPost(elem_iter start, elem_iter end) {
		this->post_len_v        = GetPostContentLength();
		this->post_bytes_read_v = 0;
    }

    void ReadPostData(const char& c) {
		if (this->post_len_v == 0) Detach();
		if (this->post_bytes_read_v == 0 && is_whitespace(c)) return;
		
		if (++this->post_bytes_read_v == this->post_len_v) {
			ProcessURLEncodedStrList(std::string(Accumulator(), Accumulator() + AccumulatorLen()), &this->postvalues_v);
			Detach();
		}
	}

    void ProcessRequestMethod(elem_iter start, elem_iter end) {
		this->request_method_v = lcase(trim(std::string(start, end)));

		if (this->request_method_v != "get" && this->request_method_v != "post")
			throw std::runtime_error("'" + this->request_method_v + "' is not a supported HTTP request type.");
	}

    void ProcessURL(elem_iter start, elem_iter end) {
		string_pair URLData = lsplit<char>(std::string(start, end), "?");

		this->url_v = urldecode(trim(URLData.first));
		ProcessURLEncodedStrList(URLData.second, &this->qsvalues_v);
    }

    void ProcessHeader(elem_iter start, elem_iter end) {
		string_pair Header = lsplit<char>(std::string(start, end), ":");
		std::string key    = lcase(trim(Header.first));

		if (key != "cookie")
			this->headers_v[key] = trim(Header.second);
		else
			ProcessCookieList(Header.second, &this->cookies_v);
    }

    void ProcessCookieList(const std::string& s, http_request::named_values* pckl) {
		string_pair cck = lsplit<char>(s, ";");

		while (!cck.first.empty()) {
			string_pair            cckv = lsplit<char>(cck.first, "=");
			std::string            key  = lcase(urldecode(trim(cckv.first)));
			named_values::iterator cki  = pckl->find(key);
			
			if (cki == pckl->end())
				(*pckl)[key] = trim(replace<char>(cckv.second, "\"", ""));
			else
				cki->second = cki->second + "," + trim(replace<char>(cckv.second, "\"", ""));
			
			cck = lsplit<char>(cck.second, ";");
		}
    }

    void ProcessURLEncodedStrList(const std::string& s, http_request::named_values* pvl) {
		string_pair uesl = lsplit<char>(s, "&");
		
		while (!uesl.first.empty()) {
			string_pair            qs  = lsplit<char>(uesl.first, "=");
			std::string            key = lcase(urldecode(trim(qs.first)));
			named_values::iterator qsi = pvl->find(key);
			
			if (qsi == pvl->end())
				(*pvl)[key] = urldecode(trim(qs.second));
			else
				qsi->second = qsi->second + "," + urldecode(trim(qs.second));
			
			uesl = lsplit<char>(uesl.second, "&");
		}
    }

    unsigned int GetPostContentLength() {
		unsigned int plen = 0;

		named_values::const_iterator nvi = this->headers_v.find("content-length");

		if (nvi != this->headers_v.end())
			plen = from_string<unsigned int>(nvi->second);

		return plen;
    }

    bool IsFormEncoded() {
		named_values::const_iterator hvi = this->headers_v.find("content-type");
		return (hvi != this->headers_v.end()) && (lcase(hvi->second) == "application/x-www-form-urlencoded");
    }
};

http_request::http_request(std::istream& in) {
    HTTPRequestReader reader;
    reader.Run(in);

    this->bytes_read_v     = reader.bytes_read();
    this->request_method_v = reader.method();
    this->url_v            = reader.url();
    this->headers_v        = reader.headers();
    this->cookies_v        = reader.cookies();
    this->qsvalues_v       = reader.query_string_values();
    this->postvalues_v     = reader.post_values();
}

http_request::~http_request() {
}

const std::string& http_request::method() const                             { return this->request_method_v; }
const std::string& http_request::url() const                                { return this->url_v;            }
unsigned int http_request::bytes_read() const				                { return this->bytes_read_v;     }
const http_request::named_values& http_request::headers() const             { return this->headers_v;        }
const http_request::named_values& http_request::cookies() const             { return this->cookies_v;        }
const http_request::named_values& http_request::query_string_values() const { return this->qsvalues_v;       }
const http_request::named_values& http_request::post_values() const         { return this->postvalues_v;     }

// read an HTTP response
enum HTTPRespReadState {
	ReadingResponseHead,
	ReadingResponseCode,
	ReadingResponseState,
	ReadingResponseHeader,
	ReadingResponseData
};

class HTTPResponseReader;
typedef StreamFSM<char, HTTPRespReadState, HTTPResponseReader> HTTPResponseReaderMachine;

class HTTPResponseReader : public http_response, public HTTPResponseReaderMachine {
public:
	HTTPResponseReader() : HTTPResponseReaderMachine(ReadingResponseHead), found_non_ws(false) {
		// 'bytes read' comes from HTTPResponseData - it's supposed to indicate the total number of
		// read in processing the response
		this->bytes_read_v = 0;

		AddStateHandler(ReadingResponseHead,   &HTTPResponseReader::ReadResponseHead);
		AddStateHandler(ReadingResponseCode,   &HTTPResponseReader::ReadResponseCode);
		AddStateHandler(ReadingResponseState,  &HTTPResponseReader::ReadResponseState);
		AddStateHandler(ReadingResponseHeader, &HTTPResponseReader::ReadResponseHeader);
		AddStateHandler(ReadingResponseData,   &HTTPResponseReader::ReadResponseData);
		
		AddTransitionHandler(AnyState,              AnyState,              &HTTPResponseReader::CountBytesReceived);
		AddTransitionHandler(ReadingResponseCode,   AnyState,              &HTTPResponseReader::ProcessResponseCode);
		AddTransitionHandler(ReadingResponseState,  AnyState,              &HTTPResponseReader::ProcessResponseState);
		AddTransitionHandler(AnyState,              ReadingResponseData,   &HTTPResponseReader::PrepForData);
		AddTransitionHandler(ReadingResponseHeader, ReadingResponseHeader, &HTTPResponseReader::ProcessHeader);
    }
private:
    unsigned int data_len_v;
    unsigned int data_bytes_read_v;
    bool         found_non_ws;

    void CountBytesReceived(elem_iter start, elem_iter end) {
		this->bytes_read_v += end - start;
    }

    void ReadResponseHead(const char& c) {
		if (!is_whitespace(c) && !this->found_non_ws) {
		    this->found_non_ws = true;
		} else if (this->found_non_ws && is_whitespace(c)) {
			this->found_non_ws = false;
			SetState(ReadingResponseCode);
		}
    }

    void ReadResponseCode(const char& c) {
		if (!is_whitespace(c) && !this->found_non_ws) {
			this->found_non_ws = true;
		} else if (this->found_non_ws && is_whitespace(c)) {
			this->found_non_ws = false;
			SetState(ReadingResponseState);
		}
    }

    void ReadResponseState(const char& c) {
		if (!is_whitespace(c) && !this->found_non_ws) {
			this->found_non_ws = true;
		}
	
		if (c == '\n') {
			this->found_non_ws = false;
			SetState(ReadingResponseHeader);
		}
    }

    void ReadResponseHeader(const char& c) {
		if (!is_whitespace(c) && !this->found_non_ws) {
			this->found_non_ws = true;
		}

		if ( (c == '\n') && !this->found_non_ws ) {
			this->found_non_ws = false;

			// we've read the last header, read data
			SetState(ReadingResponseData);
		} else if (c == '\n') {
			// we're at the end of a header, move to the next one
			this->found_non_ws = false;
			SetState(ReadingResponseHeader);
		}
    }

    void PrepForData(elem_iter start, elem_iter end) {
		this->data_len_v        = GetDataContentLength();
		this->data_bytes_read_v = 0;
    }

    void ReadResponseData(const char& c) {
		if (this->data_len_v == 0) Detach();
		if (this->data_bytes_read_v == 0 && is_whitespace(c)) return;
		
		if (++this->data_bytes_read_v == this->data_len_v) {
			this->body_v = std::string(Accumulator(), Accumulator() + AccumulatorLen());
			Detach();
		}
	}

    void ProcessResponseCode(elem_iter start, elem_iter end) {
		this->code_v = trim(std::string(start, end));
	}

    void ProcessResponseState(elem_iter start, elem_iter end) {
		this->state_v = trim(std::string(start, end));
    }

    void ProcessHeader(elem_iter start, elem_iter end) {
		string_pair Header = lsplit<char>(std::string(start, end), ":");
		std::string key    = lcase(trim(Header.first));

		this->headers_v[key] = trim(Header.second);
    }

    unsigned int GetDataContentLength() {
		unsigned int plen = 0;

		named_values::const_iterator nvi = this->headers_v.find("content-length");

		if (nvi != this->headers_v.end())
			plen = from_string<unsigned int>(nvi->second);

		return plen;
    }
};

http_response::http_response(std::istream& in) {
	HTTPResponseReader reader;
	reader.Run(in);

	this->bytes_read_v = reader.bytes_read();
	this->code_v       = reader.code();
	this->state_v      = (static_cast<http_response&>(reader)).state();
	this->headers_v    = reader.headers();
	this->body_v       = reader.body();
}

http_response::~http_response() {
}

const std::string& http_response::code() const { return this->code_v; }
const std::string& http_response::state() const { return this->state_v; }
unsigned int http_response::bytes_read() const { return this->bytes_read_v; }
const http_response::named_values& http_response::headers() const { return this->headers_v; }
const std::string& http_response::body() const { return this->body_v; }

}
