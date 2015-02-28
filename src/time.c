#include "time.h"
#include <Windows.h>

/** Returns a timestamp in milliseconds. */
double time_get() {
	LARGE_INTEGER Frequency, count;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&count);
	return (double)count.QuadPart * 1000000 / Frequency.QuadPart;
}