#include "time.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#endif

/** Returns a timestamp in milliseconds. */
double time_get() {
#ifdef _WIN32
	LARGE_INTEGER Frequency, count;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&count);
	return (double)count.QuadPart * 1000000 / Frequency.QuadPart;
#else

#endif
}
