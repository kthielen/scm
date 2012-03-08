#include <scm/eval/Allocator.hpp>
#include <scm/eval/Data.hpp>
#include <scm/eval/Util.hpp>
#include <scm/eval/CFuncBind.hpp>
#include <scm/eval/CTypeUtil.hpp>
#include <scm/str/Util.hpp>

#include <scm/eval/CBFuncBind.hpp>

#include <vector>

namespace scm {

// environment operations
Value* SEval(Value* val, EnvironmentFrame* env)               { return Eval(Eval(val, env), env); }
void   Set(Symbol* sym, Value* val, EnvironmentFrame* env)    { env->Set(sym, Eval(val, env)); }

Value* SEvalE(Value* val, Value* eenv, EnvironmentFrame* env) {
	EnvironmentFrame* nenv = assert_type<EnvironmentFrame>(Eval(eenv, env));
	return Eval(val, nenv);
}

void Define(Value* sv, ConsPair* rest, EnvironmentFrame* env) {
    if (Symbol* s = expr_cast<Symbol>(sv))
    {
		if (rest == 0)
			throw std::runtime_error("Empty definition: " + PrintExpression(s));

        if (rest->tail() != 0)
            throw std::runtime_error("Extraneous values given to define '" + PrintExpression(s) + "': " + PrintExpression(rest));
	    
        env->Define(s, Eval(rest->head(), env));
    }
    else if (ConsPair* fd = expr_cast<ConsPair>(sv))
    {
        gcguard<LambdaFunction> fn = env->allocator()->allocate<LambdaFunction>(fd->tail(), rest, env);
        gcguard<ConsPair>       ra = env->allocator()->allocate<ConsPair>(fn.ptr(), (Value*)0);

        if (expr_cast<ConsPair>(fd->head()) != 0)
            throw std::runtime_error("Nested function definitions not supported with 'Define': " + PrintExpression(sv) + " -> " + PrintExpression(rest));

        Define(fd->head(), ra.ptr(), env);
    }
    else
    {
        throw std::runtime_error("Define expected a symbol or function declaration to define but got '" + PrintExpression(sv) + "' instead.");
    }
}

Value* LEnv(EnvironmentFrame* env) { return env; }

Value* LEnvSeq(EnvironmentFrame* env) {
    if (env == 0) return 0;

    typedef std::vector<Value*> RetV;
    const EnvironmentFrame::VarFrame& evf = env->value();
    RetV result;

    for (EnvironmentFrame::VarFrame::const_iterator evfi = evf.begin(); evfi != evf.end(); ++evfi)
	result.push_back(env->allocator()->allocate<ConsPair>(const_cast<Symbol*>(evfi->first), evfi->second));
    
    return env->allocator()->allocate<ConsPair>(LiftContainer(result, env->allocator()), LEnv(env->Parent()));;
}

void LEnvJoin(Value* e, EnvironmentFrame* env) {
	EnvironmentFrame* fe = assert_type<EnvironmentFrame>(Eval(e, env));
    const EnvironmentFrame::VarFrame& evf = fe->value();
    for (EnvironmentFrame::VarFrame::const_iterator evfi = evf.begin(); evfi != evf.end(); ++evfi) {
		env->Define(evfi->first, evfi->second);
	}
}

double LPtrV(Value* v, EnvironmentFrame* env) {
	return (long long)(Eval(v, env));	
}

// the lambda function generator
Value* Lambda(Value* argl, ConsPair* def, EnvironmentFrame* env) { return env->allocator()->allocate<LambdaFunction>(argl, def, env); }

// basic 'special forms'
Value* Quote(Value* v)                                            { return v; }
void   While(Value* pred, ConsPair* body, EnvironmentFrame* env)  { while (Eval(pred, env) != 0) EvalForEach(body, env); }
void   Raise(std::string error_msg)                               { throw std::runtime_error(error_msg); }

Value* If(bool f, ConsPair* consalt, EnvironmentFrame* env)
{
    if (consalt == 0)
        throw std::runtime_error("If expects a consequence.");
    
    if (f)
        return Eval(consalt->head(), env);
    else if (consalt->tail() == 0)
        return 0;

    ConsPair* cr = assert_type<ConsPair>(consalt->tail());
    if (cr->tail() != 0) throw std::runtime_error("Too many arguments given to 'if'.");
    
    return Eval(cr->head(), env);
}

Value* Try(Value* expr, Value* catcher, EnvironmentFrame* env)
{
    try
    {
	return Eval(expr, env);
    }
    catch (std::runtime_error& e)
    {
	gcguard<Function> fn   = assert_type<Function>(Eval(catcher, env));
	gcguard<ConsPair> argl = env->allocator()->allocate<ConsPair>(env->allocator()->allocate<String>(e.what()), (Value*)0);
	return fn->Invoke(argl.ptr(), env);
    }
}

Value* Case(Value* inst, ConsPair* patterns, EnvironmentFrame* env)
{
    gcguard<Value>            evinst      = Eval(inst, env);
    gcguard<EnvironmentFrame> match_frame = env->allocator()->allocate<EnvironmentFrame>(env);

    while (patterns != 0)
    {
        ConsPair* pattp = assert_type<ConsPair>(patterns->head());

        if (pattp == 0)
            throw std::runtime_error("Case expected a pattern->implication pair but got nil.");
        else
            match_frame->Clear(); // prepare for the next match

        if (Match(Eval(pattp->head(), env), evinst.ptr(), match_frame.ptr()))
            return EvalForEach(pattp->tail(), match_frame.ptr());

        patterns = expr_cast<ConsPair>(patterns->tail());
    }

    return 0;
}

Value* Cond(ConsPair* cases, EnvironmentFrame* env)
{
    while (cases != 0)
    {
	ConsPair* pcase = assert_type<ConsPair>(cases->head());
	
	if (pcase == 0)
	    throw std::runtime_error("Cond expected a predicate->implication pair but got nil.");
	else if (Eval(pcase->head(), env) != 0)
	    return EvalForEach(pcase->tail(), env);
	else
	    cases = expr_cast<ConsPair>(cases->tail());
    }

    return 0;
}

// boolean operators
Value* andp(ConsPair* vals, EnvironmentFrame* env)
{
    Value* rv = 0;
    
    while (vals != 0)
    {
	rv = Eval(vals->head(), env);
	if (rv == 0) return 0;
	
        vals = expr_cast<ConsPair>(vals->tail());
    }
    
    return rv == 0 ? alloc_symbol(env->allocator(), "t") : rv;
}

Value* orp(ConsPair* vals, EnvironmentFrame* env)
{
    while (vals != 0)
    {
	Value* tv = Eval(vals->head(), env);
	if (tv != 0) return tv;

	vals = expr_cast<ConsPair>(vals->tail());
    }

    return 0;
}

bool notp(Value* v, EnvironmentFrame* env) { return Eval(v, env) == 0; }

// the folds
Value* FoldL(Value* iv, Value* fn, Value* lst, EnvironmentFrame* env)
{
    gcguard<Value>    result = Eval(iv, env);
    gcguard<Function> fnv    = assert_type<Function>(Eval(fn, env));
    gcguard<ConsPair> lstv   = assert_type<ConsPair>(Eval(lst, env));
    gcguard<ConsPair> arg2   = env->allocator()->allocate<ConsPair>((Value*)0, (Value*)0);
    gcguard<ConsPair> arg1   = env->allocator()->allocate<ConsPair>((Value*)0, arg2.ptr());

    gcguard<ConsPair> qa1    = env->allocator()->allocate<ConsPair>((Value*)0, (Value*)0);
    gcguard<ConsPair> quote1 = env->allocator()->allocate<ConsPair>(bindfn(env->allocator(), &Quote), qa1.ptr());

    gcguard<ConsPair> qa2    = env->allocator()->allocate<ConsPair>((Value*)0, (Value*)0);
    gcguard<ConsPair> quote2 = env->allocator()->allocate<ConsPair>(bindfn(env->allocator(), &Quote), qa2.ptr());

    while (lstv.ptr() != 0)
    {
        qa1->head(result.ptr());
        qa2->head(lstv->head());

        arg1->head(quote1.ptr());
        arg2->head(quote2.ptr());

        result = fnv->Invoke(arg1.ptr(), env);
        lstv   = assert_type<ConsPair>(lstv->tail());
    }

    return result.ptr();
}

Value* FoldR(Value* iv, Value* fn, Value* lst, EnvironmentFrame* env)
{
    gcguard<Value>    result = Eval(iv, env);
    gcguard<Function> fnv    = assert_type<Function>(Eval(fn, env));
    gcguard<ConsPair> lstv   = assert_type<ConsPair>(Eval(lst, env));
    gcguard<ConsPair> arg2   = env->allocator()->allocate<ConsPair>((Value*)0, (Value*)0);
    gcguard<ConsPair> arg1   = env->allocator()->allocate<ConsPair>((Value*)0, arg2.ptr());

    gcguard<ConsPair> qa1    = env->allocator()->allocate<ConsPair>((Value*)0, (Value*)0);
    gcguard<ConsPair> quote1 = env->allocator()->allocate<ConsPair>(bindfn(env->allocator(), &Quote), qa1.ptr());

    gcguard<ConsPair> qa2    = env->allocator()->allocate<ConsPair>((Value*)0, (Value*)0);
    gcguard<ConsPair> quote2 = env->allocator()->allocate<ConsPair>(bindfn(env->allocator(), &Quote), qa2.ptr());

    typedef std::vector<Value*> ValSeq;
    ValSeq prefold_vals;

    // accumulate pre-fold vals
    ConsPair* lvnode = lstv.ptr();
    
    while (lvnode != 0)
    {
	prefold_vals.push_back(lvnode->head());
	lvnode = expr_cast<ConsPair>(lvnode->tail());
    }

    // now fold-right
    for (ValSeq::reverse_iterator rvi = prefold_vals.rbegin(); rvi != prefold_vals.rend(); ++rvi)
    {
	qa1->head(*rvi);
	qa2->head(result.ptr());

	arg1->head(quote1.ptr());
	arg2->head(quote2.ptr());

	result = fnv->Invoke(arg1.ptr(), env);
    }

    return result.ptr();
}

Value* Unfold(Value* stopfn, Value* mapfn, Value* nextfn, Value* seed, EnvironmentFrame* env)
{
    // eval/decode input arguments
    gcguard<Function> estopfn = assert_type<Function>(Eval(stopfn, env));
    gcguard<Function> emapfn  = assert_type<Function>(Eval(mapfn,  env));
    gcguard<Function> enextfn = assert_type<Function>(Eval(nextfn, env));
    gcguard<Value>    eseed   = Eval(seed, env);

    // make an inner-eval thunk
    gcguard<ConsPair> a1    = env->allocator()->allocate<ConsPair>((Value*)0, (Value*)0);
    gcguard<ConsPair> quote = env->allocator()->allocate<ConsPair>(bindfn(env->allocator(), &Quote), a1.ptr());
    gcguard<ConsPair> argl  = env->allocator()->allocate<ConsPair>(quote.ptr(), (Value*)0);

    // unfold the seed value until we've got to stop
    typedef std::vector< gcguard<Value> > ValSeq;
    ValSeq result;

    while (true)
    {
        // do we need to stop unfolding?
        a1->head(eseed.ptr());
        if (estopfn->Invoke(argl.ptr(), env) != 0) break;

        // add the mapped seed to the result
        result.push_back(emapfn->Invoke(argl.ptr(), env));

        // change the seed
        eseed = enextfn->Invoke(argl.ptr(), env);
    }

    return LiftContainer(result, env->allocator());
}

Value* Iterate(Value* seed, Value* stopfn, Value* nextfn, Value* blockfn, EnvironmentFrame* env)
{
    // eval/decode input arguments
    gcguard<Function> estopfn  = assert_type<Function>(Eval(stopfn,  env));
    gcguard<Function> eblockfn = assert_type<Function>(Eval(blockfn, env));
    gcguard<Function> enextfn  = assert_type<Function>(Eval(nextfn,  env));
    gcguard<Value>    eseed    = Eval(seed, env);

    // make an inner-eval thunk
    gcguard<ConsPair> a1    = env->allocator()->allocate<ConsPair>((Value*)0, (Value*)0);
    gcguard<ConsPair> quote = env->allocator()->allocate<ConsPair>(bindfn(env->allocator(), &Quote), a1.ptr());
    gcguard<ConsPair> argl  = env->allocator()->allocate<ConsPair>(quote.ptr(), (Value*)0);

    // iterate until we've got to stop
    gcguard<Value> result = 0;
    
    while (true)
    {
        // do we need to stop?
        a1->head(eseed.ptr());
        if (estopfn->Invoke(argl.ptr(), env) != 0) break;

        // evaluate the iteration block
        result = eblockfn->Invoke(argl.ptr(), env);

        // change the seed
        eseed = enextfn->Invoke(argl.ptr(), env);
    }

    return result.ptr();
}

// a basic type introspection facility
Value* LTypeID(Value* v, EnvironmentFrame* env, Allocator* a)
{
    Value*      ev   = Eval(v, env);
    std::string tsym = "unknown";
    
    if (ev == 0 || expr_cast<ConsPair>(ev) != 0)
	tsym = "pair";
    else if (expr_cast<Number>(ev) != 0)
	tsym = "number";
    else if (expr_cast<String>(ev) != 0)
	tsym = "string";
    else if (expr_cast<Symbol>(ev) != 0)
	tsym = "symbol";
    else if (expr_cast<DateTime>(ev) != 0)
	tsym = "datetime";
    else if (expr_cast<Function>(ev) != 0)
	tsym = "function";

    return alloc_symbol(a, tsym);
}

Value* MakeArray(unsigned int n, Allocator* a) { return a->allocate<ValArray>(n, a); }
Value* GetArrayElem(Value* av, unsigned int n, EnvironmentFrame* env) { return assert_type<ValArray>(Eval(av, env))->get(n); }
void   SetArrayElem(Value* av, unsigned int n, Value* v, EnvironmentFrame* env) { assert_type<ValArray>(Eval(av, env))->set(n, Eval(v, env)); }
Value* ArraySize(Value* av, EnvironmentFrame* env)                              { return assert_type<ValArray>(Eval(av, env))->size(); }

// base importer
void InitBaseStdEnv(EnvironmentFrame* env)
{
    Allocator* alloc = env->allocator();

    // allocate standard environment functions
    env->Define("eval",             bindfn(alloc, &SEval));
    env->Define("eval/env",         bindfn(alloc, &SEvalE));
    env->Define("define",           bindfn(alloc, &Define));
    env->Define("set",              bindfn(alloc, &Set));
    env->Define("environment",      bindfn(alloc, &LEnv));
	env->Define("use",              bindfn(alloc, &LEnvJoin));
    env->Define("open-environment", bindfn(alloc, &LEnvSeq));
	env->Define("ptr",              bindfn(alloc, &LPtrV));

    // allocate lambda-the-ultimate
    env->Define("lambda", bindfn(alloc, &Lambda));

    // allocate special forms
    env->Define("quote", bindfn(alloc, &Quote));
    env->Define("if",    bindfn(alloc, &If));
    env->Define("while", bindfn(alloc, &While));
    env->Define("raise", bindfn(alloc, &Raise));
    env->Define("try",   bindfn(alloc, &Try));
    env->Define("case",  bindfn(alloc, &Case));
    env->Define("cond",  bindfn(alloc, &Cond));

    // allocate boolean operators
    env->Define("and", bindfn(alloc, &andp));
    env->Define("or",  bindfn(alloc, &orp));
    env->Define("not", bindfn(alloc, &notp));

    // the folds
    env->Define("foldl",   bindfn(alloc, &FoldL));
    env->Define("foldr",   bindfn(alloc, &FoldR));
    env->Define("unfold",  bindfn(alloc, &Unfold));
    env->Define("iterate", bindfn(alloc, &Iterate));

	// an array data type
	env->Define("make-array", bindfn(alloc, &MakeArray));
	env->Define("array-get",  bindfn(alloc, &GetArrayElem));
	env->Define("array-set",  bindfn(alloc, &SetArrayElem));
	env->Define("array-size", bindfn(alloc, &ArraySize));

    // misc.
    env->Define("type",         bindfn(alloc, &LTypeID));
    env->Define("profile-info", bindfn(alloc, &ProfileInfo));
}

}
