#include <scm/eval/Data.hpp>
#include <scm/eval/CFuncBind.hpp>
#include <scm/eval/CTupleUtil.hpp>
#include <scm/eval/Util.hpp>
#include <sys/time.h>
#include <time.h>

namespace scm {

double TickCount() {
    timeval tv; memset(&tv, 0, sizeof(tv));
    gettimeofday(&tv, 0);
    return (((double)tv.tv_sec) * 1000.0) + (((double)tv.tv_usec) / 1000.0);
}

Value* LCTime(Allocator* a) { return a->allocate<DateTime>(time(NULL)); }
int LCNTime()   { return time(NULL); }

std::string LFmtTime(double timev)   { time_t tv = (time_t)timev; tm* ptm = localtime(&tv); return ptm ? asctime(ptm) : ""; }
std::string LFmtGMTime(double timev) { time_t tv = (time_t)timev; tm* ptm = gmtime(&tv);    return ptm ? asctime(ptm) : ""; }

typedef tuple6<int, int, int, int, int, int> DateTimeDec;

DateTimeDec decode_gmtime(int timv) {
	time_t tv  = (time_t)timv;
	tm*    ptm = gmtime(&tv);
	return DateTimeDec(ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
}

DateTimeDec decode_time(int timv) {
	time_t tv  = (time_t)timv;
	tm*    ptm = localtime(&tv);
	return DateTimeDec(ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
}

void InitDateTimeStdEnv(EnvironmentFrame* env)
{
    Allocator* alloc = env->allocator();

    env->Define("now",           bindfn(alloc, &LCTime));
    env->Define("ctime",         bindfn(alloc, &LCNTime));
    env->Define("format-time",   bindfn(alloc, &LFmtTime));
    env->Define("format-gmtime", bindfn(alloc, &LFmtGMTime));
	env->Define("decode-gmtime", bindfn(alloc, &decode_gmtime));
	env->Define("decode-time",   bindfn(alloc, &decode_time));
    env->Define("tick",          bindfn(alloc, &TickCount));
}

}
