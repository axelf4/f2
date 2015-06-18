#include "time.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#endif

/** Returns a timestamp in milliseconds. */
float time_get() {
#ifdef _WIN32
	LARGE_INTEGER frequency, count;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&count);
	return (float) (count.QuadPart / (frequency.QuadPart / 1000));
#else
	/*struct timespec tp;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp);
	return tp.tv_nsec;*/
	return 0;
#endif
}