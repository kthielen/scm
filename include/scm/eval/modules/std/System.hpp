#ifndef EVAL_STDMODULE_SYSTEM_H_INCLUDED
#define EVAL_STDMODULE_SYSTEM_H_INCLUDED

#include <scm/eval/Data.hpp>

namespace scm {

void InitSystemStdEnv(EnvironmentFrame* env);

std::string getPWD();

}

#endif
