#include "filesystemwatch.h"

int listenforchanges(
#ifdef _WIN32
	LPCTSTR lpPathName,
#else
	const char *pathname,
#endif
	void(*callback)()) {
#ifdef _WIN32

#endif
	return 0;
}