#include <scm/eval/modules/std/Lists.hpp>
#include <scm/eval/CFuncBind.hpp>
#include <scm/eval/CTypeUtil.hpp>
#include <scm/eval/Util.hpp>
#include <scm/parse/StreamFSM.hpp>

using namespace parse;

namespace scm {

Value* MakeList(ConsPair* args, EnvironmentFrame* env) { return EvalMap(args, env); }
Value* Head(Value* lst, EnvironmentFrame* env)         { return assert_non_nil_type<ConsPair>(Eval(lst, env))->head(); }
Value* Tail(Value* lst, EnvironmentFrame* env)         { return assert_non_nil_type<ConsPair>(Eval(lst, env))->tail(); }

Value* Pair(Value* h, Value* t, EnvironmentFrame* env, Allocator* a)
{
    gcguard<Value> head = Eval(h, env);
    gcguard<Value> tail = Eval(t, env);

    return a->allocate<ConsPair>(head.ptr(), tail.ptr());
}

double LLength(Value* v, EnvironmentFrame* env)
{
    Value* lv = Eval(v, env);

    if (String* sv = expr_cast<String>(lv))
    {
	return sv->value().size();
    }
    else if (ConsPair* cp = expr_cast<ConsPair>(lv))
    {
	int c = 0;
	while (cp != 0) { cp = expr_cast<ConsPair>(cp->tail()); ++c; }
	return c;
    }
    else if (lv == 0)
    {
	return 0;
    }
    else
    {
	throw std::runtime_error("Length expects a list or a string.");
    }
}

Value* LDAppend(Value* lst, Value* dv, EnvironmentFrame* env)
{
    gcguard<ConsPair> elst  = assert_non_nil_type<ConsPair>(Eval(lst, env));
    ConsPair*         rootp = elst.ptr();
    ConsPair*         lastp = elst.ptr();
    gcguard<Value>    ev    = Eval(dv, env);

    while (rootp != 0)
    {
	lastp = rootp;
	rootp = expr_cast<ConsPair>(rootp->tail());
    }

    lastp->tail(ev.ptr());
    return elst.ptr();
}

Value* LDSetCar(Value* lst, Value* dv, EnvironmentFrame* env)
{
    gcguard<ConsPair> elst = assert_non_nil_type<ConsPair>(Eval(lst, env));
    elst->head(Eval(dv, env));
    return elst.ptr();
}

Value* LDSetCdr(Value* lst, Value* dv, EnvironmentFrame* env)
{
    gcguard<ConsPair> elst = assert_non_nil_type<ConsPair>(Eval(lst, env));
    elst->tail(Eval(dv, env));
    return elst.ptr();
}

Value* LIn(Value* val, Value* lst, EnvironmentFrame* env, Allocator* a)
{
    gcguard<Value> ev   = Eval(val, env);
    ConsPair*      elst = assert_type<ConsPair>(Eval(lst, env));
    int            idx  = 0;
    
    while (elst != 0)
    {
	Value* elstv = elst->head();

	if (elstv == ev.ptr())
	    return a->allocate<Number>(idx);
	else if (elstv != 0 && ev.ptr() != 0 && *elstv == *ev)
	    return a->allocate<Number>(idx);
    
	++idx;
	elst = expr_cast<ConsPair>(elst->tail());
    }
    
    return 0;
}

void InitListsStdEnv(EnvironmentFrame* env)
{
    Allocator* alloc = env->allocator();
    
    env->Define("list",     bindfn(alloc, &MakeList));
    env->Define("pair",     bindfn(alloc, &Pair));
    env->Define("head",     bindfn(alloc, &Head));
    env->Define("tail",     bindfn(alloc, &Tail));
    env->Define("in",       bindfn(alloc, &LIn));
    env->Define("length",   bindfn(alloc, &LLength));
    env->Define("dappend",  bindfn(alloc, &LDAppend));
    env->Define("set-head", bindfn(alloc, &LDSetCar));
    env->Define("set-tail", bindfn(alloc, &LDSetCdr));
}

}
