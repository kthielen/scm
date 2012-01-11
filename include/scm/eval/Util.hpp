#ifndef EVAL_UTIL_H_INCLUDED
#define EVAL_UTIL_H_INCLUDED

#include <scm/eval/Allocator.hpp>
#include <scm/eval/Data.hpp>
#include <scm/eval/SExprReader.hpp>
#include <scm/parse/StreamFSM.hpp>
#include <scm/str/Util.hpp>
#include <iostream>
#include <stdexcept>

namespace scm {

Value* ProfileInfo(Allocator* a);

inline ConsPair* List(Allocator* a) { return (ConsPair*)0; }
inline ConsPair* List(Allocator* a, Value* v0) { return a->allocate<ConsPair>(v0, List(a)); }
inline ConsPair* List(Allocator* a, Value* v0, Value* v1) { return a->allocate<ConsPair>(v0, List(a, v1)); }
inline ConsPair* List(Allocator* a, Value* v0, Value* v1, Value* v2) { return a->allocate<ConsPair>(v0, List(a, v1, v2)); }
inline ConsPair* List(Allocator* a, Value* v0, Value* v1, Value* v2, Value* v3) { return a->allocate<ConsPair>(v0, List(a, v1, v2, v3)); }
inline ConsPair* List(Allocator* a, Value* v0, Value* v1, Value* v2, Value* v3, Value* v4) { return a->allocate<ConsPair>(v0, List(a, v1, v2, v3, v4)); }
inline ConsPair* List(Allocator* a, Value* v0, Value* v1, Value* v2, Value* v3, Value* v4, Value* v5) { return a->allocate<ConsPair>(v0, List(a, v1, v2, v3, v4, v5)); }
inline ConsPair* List(Allocator* a, Value* v0, Value* v1, Value* v2, Value* v3, Value* v4, Value* v5, Value* v6) { return a->allocate<ConsPair>(v0, List(a, v1, v2, v3, v4, v5, v6)); }

Value* Nth(Value* v, unsigned int n);
ConsPair* NthPair(Value* v, unsigned int n);
unsigned int Length(ConsPair* v);
ConsPair* Append(ConsPair* v1, ConsPair* v2);
ConsPair* DAppend(ConsPair* lst, ConsPair* dv);

Value* Eval(Value* expr, EnvironmentFrame* env);
ConsPair* EvalMap(ConsPair* exprs, EnvironmentFrame* env);
Value* EvalForEach(Value* exprs, EnvironmentFrame* env);
bool Match(Value* pattern, Value* instance, EnvironmentFrame* fill_env);
Value* EnQuote(Value* v, Allocator* a);
ConsPair* EnList(Value* v, Allocator* a);

class DefaultEnvironment
{
public:
	DefaultEnvironment();

	operator EnvironmentFrame*();
	EnvironmentFrame* frame();
	Allocator&        allocator();
private:
	EnvironmentFrame* env;
	Allocator         alloc;
};

void REPL(std::istream& input, std::ostream& output, EnvironmentFrame* e, const std::string& prompt = "> ", const std::string& line_end = "\n");
void REPL(std::istream& input, std::ostream& output, const std::string& prompt = "> ", const std::string& line_end = "\n");

template <typename T>
    std::string normalized_type_name()
    {
#ifdef WIN32
        std::string lrcut = str::rsplit<char>(std::string(typeid(T).name()), "eval").second;
        return std::string(lrcut.begin() + 2, lrcut.end());
#else
        return str::cxx_demangle<char>(typeid(T).name());
        return std::string(typeid(T).name());
        std::string lrcut = str::rsplit<char>(std::string(typeid(T).name()), "eval").second;
        return std::string(lrcut.begin() + 1, lrcut.end() - 1);
#endif
    }

template <typename T>
    struct TypeTraits
    {
        static const bool allow_null = false;
    };

template <>
    struct TypeTraits<ConsPair>
    {
        static const bool allow_null = true;
    };

template <typename T>
    T* assert_type(Value* v)
    {
        T* r = expr_cast<T>(v);

        if (r == 0 && (v != 0 || !TypeTraits<T>::allow_null))
            throw std::runtime_error("Expected a value of type '" + normalized_type_name<T>() + "' in place of the value '" + PrintExpression(v) + "'.");

        return r;
    }

// a strict assert_type that requires non-nil values for lists
template <typename T>
    T* assert_non_nil_type(Value* v)
    {
        T* r = expr_cast<T>(v);

        if (r == 0)
            throw std::runtime_error("Expected a value of type '" + normalized_type_name<T>() + "' in place of the value '" + PrintExpression(v) + "'.");

        return r;
    }

template <int N>
    void assert_length(ConsPair* p)
    {
        if (N > 0 && Length(p) != N)
            throw std::runtime_error("Expected " + str::to_string(N) + " arguments but received " + str::to_string(Length(p)) + ".");
    }

inline Symbol* alloc_symbol(Allocator* a, const std::string& s) { return a->flyweight_allocate<Symbol>(str::lcase(s)); }

template <typename T>
	inline const T& trace(const std::string& msg, const T& t) {
		std::cout << msg << t << std::endl;
		return t;
	}

// useful for generating bindings to standard operators or constructors
template <typename C> C gen_ctor0() { return C(); }
template <typename C, typename A0> C gen_ctor1(const A0& a0) { return C(a0); }
template <typename C, typename A0, typename A1> C gen_ctor2(const A0& a0, const A1& a1) { return C(a0, a1); }
template <typename C, typename A0, typename A1, typename A2> C gen_ctor3(const A0& a0, const A1& a1, const A2& a2) { return C(a0, a1, a2); }
template <typename C, typename A0, typename A1, typename A2, typename A3> C gen_ctor4(const A0& a0, const A1& a1, const A2& a2, const A3& a3) { return C(a0, a1, a2, a3); }
template <typename C, typename A0, typename A1, typename A2, typename A3, typename A4> C gen_ctor4(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4) { return C(a0, a1, a2, a3, a4); }
template <typename C, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5> C gen_ctor4(const A0& a0, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5) { return C(a0, a1, a2, a3, a4, a5); }

template <typename A, typename B, typename C> C gen_add(const A& a, const B& b) { return a + b; }
template <typename A, typename B, typename C> C gen_sub(const A& a, const B& b) { return a - b; }
template <typename A, typename B, typename C> C gen_mul(const A& a, const B& b) { return a * b; }
template <typename A, typename B, typename C> C gen_div(const A& a, const B& b) { return a / b; }

}

#endif
