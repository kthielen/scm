#include <scm/eval/Data.hpp>
#include <scm/eval/CFuncBind.hpp>
#include <scm/eval/CTupleUtil.hpp>
#include <scm/eval/Util.hpp>
#include <time.h>

#ifdef WIN32
#include <windows.h>
#else
//#include <boost/date_time/posix_time/posix_time.hpp>

//using namespace boost::gregorian;
//using namespace boost::posix_time;
#endif

namespace scm {

#ifndef WIN32

/*
Value* LCTime(Allocator* a) { return a->allocate<DateTime>(time(NULL)); }
int    TickCount()          { static ptime reft = microsec_clock::local_time(); return (microsec_clock::local_time() - reft).total_milliseconds(); }

double LKDBDate(const ptime& v)
{
    tm tmv = to_tm(v);
    return (mktime(&tmv) / 8.64e4) - 10957.0;
}

ptime LMakeDateTime(int years, int months, int days, int hours, int minutes, int seconds)
{
    time_t       tnow   = time(NULL);
    unsigned int tzdiff = mktime(gmtime(&tnow)) - mktime(localtime(&tnow));

    tm cnvt;
    cnvt.tm_year  = years - 1900;
    cnvt.tm_mon   = months - 1;
    cnvt.tm_mday  = days;
    cnvt.tm_hour  = hours;
    cnvt.tm_min   = minutes;
    cnvt.tm_sec   = seconds;
    cnvt.tm_isdst = -1;

    return from_time_t(mktime(&cnvt) - tzdiff);
}

bool DTLT (const ptime& dt1, const ptime& dt2) { return dt1 <  dt2; }
bool DTLTE(const ptime& dt1, const ptime& dt2) { return dt1 <= dt2; }
bool DTGT (const ptime& dt1, const ptime& dt2) { return dt1 >  dt2; }
bool DTGTE(const ptime& dt1, const ptime& dt2) { return dt1 >= dt2; }

void InitDateTimeStdEnv(EnvironmentFrame* env)
{
    Allocator* alloc = env->allocator();

    env->Define("now",           bindfn(alloc, &LCTime));
    env->Define("make-datetime", bindfn(alloc, &LMakeDateTime));
    env->Define("kdb-date",      bindfn(alloc, &LKDBDate));
    env->Define("tick",          bindfn(alloc, &TickCount));

    env->Define("dt<",           bindfn(alloc, &DTLT));
    env->Define("dt<=",          bindfn(alloc, &DTLTE));
    env->Define("dt>",           bindfn(alloc, &DTGT));
    env->Define("dt>=",          bindfn(alloc, &DTGTE));
}

int TickCount()
{
    static boost::posix_time::ptime reft = boost::posix_time::microsec_clock::local_time();
    return (boost::posix_time::microsec_clock::local_time() - reft).total_milliseconds();
}
*/
int TickCount() { return 0; }

Value* LCTime(Allocator* a) { return a->allocate<DateTime>(time(NULL)); }
int LCNTime()   { return time(NULL); }

std::string LFmtTime(double timev)   { time_t tv = (time_t)timev; tm* ptm = localtime(&tv); return ptm ? asctime(ptm) : ""; }
std::string LFmtGMTime(double timev) { time_t tv = (time_t)timev; tm* ptm = gmtime(&tv);    return ptm ? asctime(ptm) : ""; }

#else

Value* LCTime(Allocator* a) { return a->allocate<DateTime>(time(NULL)); }
int LCNTime()   { return time(NULL); }
int TickCount() { return GetTickCount(); }

std::string LFmtTime(double timev)   { time_t tv = (time_t)timev; tm* ptm = localtime(&tv); return ptm ? asctime(ptm) : ""; }
std::string LFmtGMTime(double timev) { time_t tv = (time_t)timev; tm* ptm = gmtime(&tv);    return ptm ? asctime(ptm) : ""; }

#endif

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
