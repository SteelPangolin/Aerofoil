#include "GpSystemServices_POSIX.h"

#include "GpMutex_Cpp11.h"
#include "GpThreadEvent_Cpp11.h"

#include <time.h>
#include <unistd.h>

GpSystemServices_POSIX::GpSystemServices_POSIX()
{
}

int64_t GpSystemServices_POSIX::GetTime() const
{
	time_t t = time(nullptr);
	return static_cast<int64_t>(t) - 2082844800;
}

void GpSystemServices_POSIX::GetLocalDateTime(unsigned int &year, unsigned int &month, unsigned int &day, unsigned int &hour, unsigned int &minute, unsigned int &second) const
{
	time_t t = time(nullptr);
	tm *tmObject = localtime(&t);
	year = static_cast<unsigned int>(tmObject->tm_year);
	month = static_cast<unsigned int>(tmObject->tm_mon + 1);
	hour = static_cast<unsigned int>(tmObject->tm_hour);
	minute = static_cast<unsigned int>(tmObject->tm_min);
	second = static_cast<unsigned int>(tmObject->tm_sec);
}

IGpMutex *GpSystemServices_POSIX::CreateMutex()
{
	return GpMutex_Cpp11_NonRecursive::Create();
}

IGpMutex *GpSystemServices_POSIX::CreateRecursiveMutex()
{
	return GpMutex_Cpp11_Recursive::Create();
}

IGpThreadEvent *GpSystemServices_POSIX::CreateThreadEvent(bool autoReset, bool startSignaled)
{
	return GpThreadEvent_Cpp11::Create(autoReset, startSignaled);
}

uint64_t GpSystemServices_POSIX::GetFreeMemoryCosmetic() const
{
#ifdef _SC_AVPHYS_PAGES
	long pages = sysconf(_SC_AVPHYS_PAGES);
	long pageSize = sysconf(_SC_PAGE_SIZE);
	return pages * pageSize;
#else
    // macOS doesn't define _SC_AVPHYS_PAGES anyway, and this is purely cosmetic.
    // If desired, implement using FreeBSD/macOS sysctl() and HW_PHYSMEM.
    return 0;
#endif
}
