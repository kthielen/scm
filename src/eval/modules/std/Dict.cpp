#include <scm/eval/modules/std/Dict.hpp>
#include <scm/eval/CFuncBind.hpp>
#include <scm/eval/CTypeUtil.hpp>
#include <scm/eval/Util.hpp>
#include <scm/eval/Map.hpp>
#include <scm/str/Util.hpp>

using namespace str;

namespace scm {

Value* MakeDict(Value* v, EnvironmentFrame* env, Allocator* a)
{
    Value* ev = Eval(v, env);

    if (Symbol* s = expr_cast<Symbol>(ev))
    {
		if (s->value() == "symbol" || s->value() == "value")
			return a->allocate< Map<Value*> >();
		else if (s->value() == "string")
			return a->allocate< Map<std::string> >();
		else if (s->value() == "number")
			return a->allocate< Map<double> >();
    }

    return 0;
}

void InitDictStdEnv(EnvironmentFrame* env)
{
    env->Define("make-dict", bindfn(env->allocator(), &MakeDict));
}

}
