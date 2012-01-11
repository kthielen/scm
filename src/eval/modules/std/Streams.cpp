#include <scm/eval/Allocator.hpp>
#include <scm/eval/Data.hpp>
#include <scm/eval/SExprReader.hpp>
#include <scm/eval/XMLReader.hpp>
#include <scm/eval/CFuncBind.hpp>
#include <scm/eval/CTupleUtil.hpp>
#include <iostream>
#include <sstream>

//#include <scm/eval/tools/parse/gen/common.hpp>
#include <scm/stream/http_reader.hpp>
#include <scm/stream/socket_stream.hpp>
#include <scm/str/Util.hpp>

//#include <readline/readline.h>
//#include <readline/history.h>

namespace scm {

Value* ReadExpr(std::istream* input, EnvironmentFrame* env) {
    return ReadSExpression(*input, *(env->allocator()));
}

Value* ReadXMLExpr(std::istream* input, EnvironmentFrame* env) {
    return ReadXML(input, env->allocator());
}

Value* ReadHTTPRequest(std::istream* input, Allocator* alloc) {
    stream::http_request req(*input);

    std::list<Value*> ereq;
    ereq.push_back(alloc_symbol(alloc, str::lcase(req.method())));
    ereq.push_back(alloc->allocate<String>(req.url()));
    ereq.push_back(LiftContainer(req.headers(),             alloc));
    ereq.push_back(LiftContainer(req.cookies(),             alloc));
    ereq.push_back(LiftContainer(req.query_string_values(), alloc));
    ereq.push_back(LiftContainer(req.post_values(),         alloc));

    return LiftContainer(ereq, alloc);
}

Value* ReadHTTPResponse(std::istream* input, Allocator* alloc) {
	stream::http_response resp(*input);

	std::list<Value*> ereq;
	ereq.push_back(alloc->allocate<String>(resp.code()));
	ereq.push_back(alloc->allocate<String>(resp.state()));
	ereq.push_back(LiftContainer(resp.headers(), alloc));
	ereq.push_back(alloc->allocate<String>(resp.body()));

	return LiftContainer(ereq, alloc);
}

std::string ReadLine(std::istream* input) {
    std::string result;
    std::getline(*input, result);
    return result;
}

std::string Base64Enc(std::istream* input) {
	return str::base64_encode(*input);
}

/*
std::string ReadConsoleLine()
{
    char*       l = readline("");
	std::string r(l);
	
	if (l && *l) add_history(l);
	free(l);

	return r;
}
*/

std::string ReadUpTo(std::istream* input, int ch) {
    std::string result;

    char c;
    while (true)
    {
        input->get(c);
        if (c == ch || input->eof()) break;
        result.append(1, c);
    }

    return result;
}

bool StreamEOF(std::istream* stream) {
    return stream == 0 || !(*stream);
}

Value* MakeStreamBuffer(Allocator* a) { return a->allocate<OStream>(new std::ostringstream(), true); }

std::string StreamBuffString(Value* eos, EnvironmentFrame* env) {
    OStream*            os  = assert_type<OStream>(Eval(eos, env));
    std::ostringstream* oss = dynamic_cast<std::ostringstream*>(os->value());
    if (!oss) throw std::runtime_error("Expected a stream buffer to read.");

    return oss->str();
}

void JoinStreams(std::istream* input, std::ostream* output) { *output << input->rdbuf(); }

std::iostream* OpenSocket(const std::string& h, int p) {
	return new stream::socket_stream<char>(h, p);
}

void InitStreamsStdEnv(EnvironmentFrame* env) {
    Allocator* alloc = env->allocator();
    
    env->Define("read",               bindfn(alloc, &ReadExpr));
    env->Define("read-xml",           bindfn(alloc, &ReadXMLExpr));
    env->Define("read-http-request",  bindfn(alloc, &ReadHTTPRequest));
	env->Define("read-http-response", bindfn(alloc, &ReadHTTPResponse));
    env->Define("read-line",          bindfn(alloc, &ReadLine));
//	env->Define("read-console-line",  bindfn(alloc, &ReadConsoleLine));
    env->Define("read-to-char",       bindfn(alloc, &ReadUpTo));
    env->Define("eof?",               bindfn(alloc, &StreamEOF));

	env->Define("base64-encode",      bindfn(alloc, &Base64Enc));

    env->Define("stream-buffer",      bindfn(alloc, &MakeStreamBuffer));
    env->Define("buffer-string",      bindfn(alloc, &StreamBuffString));

    env->Define("join-streams",       bindfn(alloc, &JoinStreams));

	env->Define("open-socket",        bindfn(alloc, &OpenSocket));

//    add_parser_bindings(env);
}

}
