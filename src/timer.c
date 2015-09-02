#include "timer.h"
#ifdef _WIN32
#include <mmsystem.h>
#else
#define _XOPEN_SOURCE 700
#include <time.h>
#endif

/** Returns a timestamp in milliseconds. */
unsigned int getTicks() {
#ifdef _WIN32
	LARGE_INTEGER frequency, count;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&count);
	return (unsigned int) (count.QuadPart / (frequency.QuadPart / 1000));
#else
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (unsigned int) (1000 * t.tv_sec + t.tv_nsec / 1000000 + 0.5);
#endif
}
