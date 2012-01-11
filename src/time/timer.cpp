#include <scm/time/timer.hpp>

#ifndef WIN32
#	include <boost/date_time/posix_time/posix_time.hpp>
	unsigned int timer::tick_count()
	{
		static boost::posix_time::ptime reft = boost::posix_time::microsec_clock::local_time();
		return (boost::posix_time::microsec_clock::local_time() - reft).total_milliseconds();
	}
#else
#	include <windows.h>
	unsigned int timer::tick_count() { return GetTickCount(); }
#endif
