#ifndef EVAL_STDMODULE_DATETIME_H_INCLUDED
#define EVAL_STDMODULE_DATETIME_H_INCLUDED

#include <scm/eval/Data.hpp>

namespace scm {

int TickCount();

void InitDateTimeStdEnv(EnvironmentFrame* env);

}

#endif
