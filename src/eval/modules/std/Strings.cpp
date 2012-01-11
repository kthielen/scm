#include <scm/eval/Data.hpp>
#include <scm/eval/SExprReader.hpp>
#include <scm/eval/Util.hpp>
#include <scm/eval/CFuncBind.hpp>
#include <scm/str/Util.hpp>
#include <scm/parse/StreamFSM.hpp>
#include <sstream>

using namespace str;

namespace scm {

std::string concat(const std::string& s1, const std::string& s2)   { return s1 + s2; }
std::string substr(const std::string& s, int i, int f)             { return (i >= 0 && i < s.size() ? s.substr(i, f) : std::string("")); }
int         scompare(const std::string& s1, const std::string& s2) { return strcmp(s1.c_str(), s2.c_str()); }
std::string PrintExpr(Value* v, EnvironmentFrame* env)             { return PrintExpression(Eval(v, env)); }
Value*      MakeStream(const std::string& s, Allocator* a)         { return a->allocate<IStream>(new std::istringstream(s), true); }

int substr_offset(const std::string& s, const std::string& ss)
{
    std::string::size_type fl = s.find(ss);
    if (fl == std::string::npos) return -1; else return (int)fl;
}

Value* Unpack(const std::string& s, Allocator* a)
{
    if (s.empty()) return 0;

    ConsPair* r  = a->allocate<ConsPair>((Value*)0, (Value*)0);
    ConsPair* cn = r;

    int idx = 0;
    for (std::string::const_iterator si = s.begin(); si != s.end(); ++si, ++idx)
    {
	Number* n = a->allocate<Number>(static_cast<int>(*si));
	
	if (idx == 0)
	{
	    cn->head(n);
	}
	else
	{
	    ConsPair* nn = a->allocate<ConsPair>(n, (Value*)0);
	    cn->tail(nn);
	    cn = nn;
	}
    }

    return r;
}

Value* Pack(Value* v, EnvironmentFrame* env)
{
    std::string ret;
    ConsPair*   ev = assert_type<ConsPair>(Eval(v, env));

    while (ev != 0)
    {
        Number* n = assert_type<Number>(ev->head());
        ret.append(1, (char)(int)(n->value()));
        ev = assert_type<ConsPair>(ev->tail());
    }

    return env->allocator()->allocate<String>(ret);
}

void InitStringsStdEnv(EnvironmentFrame* env)
{
    Allocator* alloc = env->allocator();

    // allocate string functions
    env->Define("dy/concat",     bindfn(alloc, &concat));
    env->Define("substr",        bindfn(alloc, &substr));
    env->Define("substr-offset", bindfn(alloc, &substr_offset));
    env->Define("replace",       bindfn(alloc, &replace<char>));
    env->Define("escape",        bindfn(alloc, &escape<char>));
    env->Define("unescape",      bindfn(alloc, &unescape<char>));
    env->Define("unpack",        bindfn(alloc, &Unpack));
    env->Define("pack",          bindfn(alloc, &Pack));
    env->Define("mustendwith",   bindfn(alloc, &mustendwith<char>));
    env->Define("lsplit",        bindfn(alloc, &lsplit<char>));
    env->Define("rsplit",        bindfn(alloc, &rsplit<char>));
    env->Define("lcase",         bindfn(alloc, &lcase<char>));
    env->Define("ucase",         bindfn(alloc, &ucase<char>));
    env->Define("trim",          bindfn(alloc, &trim<char>));
    env->Define("strcmp",        bindfn(alloc, &scompare));

    env->Define("make-stream",   bindfn(alloc, &MakeStream));
    env->Define("to-string",     bindfn(alloc, &PrintExpr));
}

}
