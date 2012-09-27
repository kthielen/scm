#include <scm/eval/Allocator.hpp>
#include <scm/eval/Data.hpp>
#include <scm/eval/Util.hpp>

#include <scm/eval/modules/std/StdEnv.hpp>
#include <scm/eval/modules/std/Base.hpp>
#include <scm/eval/modules/std/Math.hpp>
#include <scm/eval/modules/std/DateTime.hpp>
#include <scm/eval/modules/std/Lists.hpp>
#include <scm/eval/modules/std/Dict.hpp>
#include <scm/eval/modules/std/Strings.hpp>
#include <scm/eval/modules/std/System.hpp>
#include <scm/eval/modules/std/FileIO.hpp>
#include <scm/eval/modules/std/Streams.hpp>
#include <scm/eval/modules/std/Prelude.hpp>

#include <iostream>

namespace scm {

// the full module loader
void InitStdModule(EnvironmentFrame* env)
{
    Allocator* alloc = env->allocator();

    // add base functions
    InitBaseStdEnv(env);

    // allocate math functions
    InitMathStdEnv(env);

    // allocate time functions
    InitDateTimeStdEnv(env);

    // allocate list functions
    InitListsStdEnv(env);

    // allocate dictionary functions
    InitDictStdEnv(env);
    
    // allocate string functions
    InitStringsStdEnv(env);

    // allocate filesystem functions
    InitFileIOStdEnv(env);

    // allocate stream functions
    InitStreamsStdEnv(env);

    // standard symbols
    env->Define("t",   alloc_symbol(alloc, "t"));
    env->Define("nil", 0);

    // as a utility, make STDIO available
    env->Define("stdin",  alloc->allocate<IStream>(&std::cin,  false));
    env->Define("stdout", alloc->allocate<OStream>(&std::cout, false));

    // import the standard prelude
    InitStdPrelude(env);

    // allocate (plugging nose) system functions
    InitSystemStdEnv(env);
}

}
