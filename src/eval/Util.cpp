#include <scm/eval/Util.hpp>
#include <scm/eval/SExprReader.hpp>
#include <scm/eval/modules/std/StdEnv.hpp>

#define SCM_DEBUG
#ifdef SCM_PROFILE
//#include <boost/date_time/posix_time/posix_time.hpp>
#include <map>

struct ProfInfo
{
    ProfInfo() : inv_count(0), total_exec_time_ms(0) { }

    unsigned int inv_count;
    unsigned int total_exec_time_ms;
};

typedef std::map<std::string, ProfInfo> ProfileData;
ProfileData profile_data;

// a simple tick counting procedure
inline unsigned int tick_count()
{
    return 0;
//	return GetTickCount();
//    static boost::posix_time::ptime reft = boost::posix_time::microsec_clock::local_time();
    
//    return (boost::posix_time::microsec_clock::local_time() - reft).total_milliseconds();
}
#endif

namespace scm {

#ifdef SCM_PROFILE

Value* ProfileInfo(Allocator* a)
{
    ConsPair* result = 0;
    ConsPair* tnode = 0;

    for (ProfileData::const_iterator pi = profile_data.begin(); pi != profile_data.end(); ++pi)
    {
        if (result == 0)
        {
            result = a->allocate<ConsPair>((Value*)0, (Value*)0);
            tnode  = result;
        }
        else
        {
            ConsPair* nnode = a->allocate<ConsPair>((Value*)0, (Value*)0);
            tnode->tail(nnode);
            tnode = nnode;
        }

        tnode->head(a->allocate<ConsPair>(a->allocate<String>(pi->first), a->allocate<ConsPair>(a->allocate<Number>(pi->second.inv_count), a->allocate<ConsPair>(a->allocate<Number>(pi->second.total_exec_time_ms), (Value*)0))));
    }

    return result;
}

#else

Value* ProfileInfo(Allocator*) { return 0; }

#endif

Value* Nth(Value* v, unsigned int n)
{
    ConsPair* cv = expr_cast<ConsPair>(v);

    unsigned int i = 0;
    while (cv != 0)
    {
        if (i == n)
            return cv->head();

	++i;

	if (i == n && cv->tail() != 0 && expr_cast<ConsPair>(cv->tail()) == 0)
	    return cv->tail();
	else
	    cv = expr_cast<ConsPair>(cv->tail());
    }

    return 0;
}

ConsPair* NthPair(Value* v, unsigned int n)
{
    ConsPair*    p = assert_type<ConsPair>(v);
    unsigned int i = 0;

    while (p != 0 && i++ < n)
        p = assert_type<ConsPair>(p->tail());

    return p;
}

unsigned int Length(ConsPair* p)
{
    int i = 0;

    while (p != 0)
    {
	++i;
	p = expr_cast<ConsPair>(p->tail());
    }

    return i;
}						

ConsPair* Append(ConsPair* v1, ConsPair* v2)
{
    // append v2 to v1 iif v1 != 0 && v2 != 0
    if (v1 == 0)
	return v2;
    else if (v2 == 0)
	return v1;

    ConsPair* r  = v1->allocator()->allocate<ConsPair>((Value*)0, (Value*)0);
    ConsPair* cn = r;
    
    while (true)
    {
	cn->head(v1->head());

	if (v1->tail() == 0)
	{
	    cn->tail(v2);
	    break;
	}

	ConsPair* np = v1->allocator()->allocate<ConsPair>((Value*)0, (Value*)0);
	cn->tail(np);
	cn = np;
	
	v1 = expr_cast<ConsPair>(v1->tail());
    }

    return r;
}

ConsPair* DAppend(ConsPair* lst, ConsPair* dv)
{
    if (lst == 0) return dv;
    if (dv  == 0) return lst;

    ConsPair* rootp = lst;
    ConsPair* lastp = lst;

    while (rootp != 0) { lastp = rootp; rootp = expr_cast<ConsPair>(rootp->tail()); }

    lastp->tail(dv);
    return lst;
}

Value* Eval(Value* expr, EnvironmentFrame* env)
{
#ifdef SCM_DEBUG
    try {
#endif

    if (expr_cast<Symbol>(expr) != 0)
        return env->Lookup(expr_cast<Symbol>(expr));
    else if (expr_cast<ConsPair>(expr) == 0)
        return expr;

    gcguard<ConsPair> cp = expr_cast<ConsPair>(expr);
    gcguard<Function> fv = assert_type<Function>(Eval(cp->head(), env));

#ifdef SCM_PROFILE
    std::string  fexp   = PrintExpression(cp->head());
    unsigned int tstart = tick_count();
#endif

    // invoke the function
    Value* result = fv->Invoke(expr_cast<ConsPair>(cp->tail()), env);

#ifdef SCM_PROFILE
    ProfInfo& pi = profile_data[fexp];
    ++(pi.inv_count);
    pi.total_exec_time_ms += (tick_count() - tstart);
#endif

    // collect garbage generated by the function (excluding the probably unreferenced result)
    env->allocator()->collect(result);

    return result;

#ifdef SCM_DEBUG
    } catch (std::runtime_error& e) { throw std::runtime_error("While evaluating: " + PrintExpression(expr) + "\n\n" + e.what()); }
#endif
}

ConsPair* EvalMap(ConsPair* exprs, EnvironmentFrame* env)
{
    if (exprs == 0) return 0;

    gcguard<ConsPair> gexprs(exprs);
    gcguard<ConsPair> result;
    ConsPair*         node = 0;

    while (exprs != 0)
    {
        Value*    rval  = Eval(exprs->head(), env);
        ConsPair* rnode = env->allocator()->allocate<ConsPair>(rval, (Value*)0);

        if (result.ptr() == 0)
        {
            result = rnode;
            node   = rnode;
        }
        else
        {
            node->tail(rnode);
            node = rnode;
        }

        exprs = expr_cast<ConsPair>(exprs->tail());
    }

    return result.ptr();
}

Value* EvalForEach(Value* exprs, EnvironmentFrame* env)
{
    Value*    result  = 0;
    ConsPair* subexpr = expr_cast<ConsPair>(exprs);
    if (subexpr == 0) return Eval(exprs, env);

    while (subexpr != 0)
    {
        gcguard<ConsPair> seg(subexpr);

        result  = Eval(subexpr->head(), env);
        subexpr = expr_cast<ConsPair>(subexpr->tail());
    }

    return result;
}

// pattern matching
bool MatchListValue(ConsPair* pattern, Value* instance, EnvironmentFrame* out_env)
{
    ConsPair* instl = expr_cast<ConsPair>(instance);

    while (pattern != 0 && instl != 0)
    {
        if (!Match(pattern->head(), instl->head(), out_env))
            return false;

        ConsPair* nextp = expr_cast<ConsPair>(pattern->tail());
        ConsPair* nexti = expr_cast<ConsPair>(instl->tail());

        if (nextp == 0)
            return Match(pattern->tail(), instl->tail(), out_env);

        pattern = nextp;
        instl   = nexti;
    }

    return pattern == instl;
}

bool MatchSymbolValue(Symbol* pattern, Value* instance, EnvironmentFrame* out_env)
{
    if (pattern->query_var())
    {
        if (out_env->HasImmediateValue(pattern->non_query_var()))
            return Match(out_env->LookupImmediateValue(pattern->non_query_var()), instance, out_env);
        else
            out_env->Define(pattern->non_query_var(), instance);

        return true;
    }
    else
    {
        return instance != 0 && *pattern == *instance;
    }
}

bool Match(Value* pattern, Value* instance, EnvironmentFrame* out_env)
{
    ConsPair* pcp = expr_cast<ConsPair>(pattern);
    if (pcp != 0)
        return MatchListValue(pcp, instance, out_env);

    Symbol* s = expr_cast<Symbol>(pattern);
    if (s != 0)
        return MatchSymbolValue(s, instance, out_env);

    // base case
    if (pattern == instance)
        return true;
    else if (pattern == 0 || instance == 0)
        return false;
    else
        return *pattern == *instance;
}

Value* EnQuote(Value* v, Allocator* a)
{
    return a->allocate<ConsPair>(alloc_symbol(a, "quote"), a->allocate<ConsPair>(v, (Value*)0));
}

ConsPair* EnList(Value* vs, Allocator* a)
{
    return a->allocate<ConsPair>(alloc_symbol(a, "list"), vs);
}

DefaultEnvironment::DefaultEnvironment()
{
	EnvironmentFrame* e = alloc.allocate<EnvironmentFrame>((EnvironmentFrame*)0);
	env = alloc.allocate<EnvironmentFrame>(e);
	alloc.root(env);

	InitStdModule(e);
}
DefaultEnvironment::operator EnvironmentFrame*()  { return frame(); }
EnvironmentFrame* DefaultEnvironment::frame()     { return env; }
Allocator&        DefaultEnvironment::allocator() { return alloc; }

void REPL(std::istream& input, std::ostream& output, EnvironmentFrame* e, const std::string& prompt, const std::string& line_end)
{
    while (input)
    {
        try
        {
            if (!prompt.empty())
                output << prompt << std::flush;

            // read
            Value* inexp = ReadSExpression(input, *(e->allocator()));
            
	    // let 'exit' kill the REPL
	    if (Symbol* s = expr_cast<Symbol>(inexp))
		if (s->value() == "exit")
		    break;
	    
            // eval
            Value* outexp = Eval(inexp, e);

            // print
            PrintExpression(outexp, output);

            // loop
            output << line_end << std::flush;
        }
        catch (std::runtime_error& e)
        {
            output << "*"               << line_end << std::flush
                   << "** " << e.what() << line_end << std::flush
                   << "*"               << line_end << std::flush;
        }
    }
}

void REPL(std::istream& input, std::ostream& output, const std::string& prompt, const std::string& line_end)
{
    Allocator alloc;
    EnvironmentFrame* root = alloc.allocate<EnvironmentFrame>((EnvironmentFrame*)0);
    alloc.root(root);
    InitStdModule(root);
    REPL(input, output, root, prompt, line_end);
}

}
