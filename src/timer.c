#include "time.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#endif

/** Returns a timestamp in milliseconds. */
long time_get() {
#ifdef _WIN32
	LARGE_INTEGER Frequency, count;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&count);
	return (long)count.QuadPart * 1000000 / Frequency.QuadPart;
#else
	/*struct timespec tp;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp);
	return tp.tv_nsec;*/
	return 0;
#endif
}