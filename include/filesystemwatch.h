#ifndef FILESYSTEMWATCH_H
#define FILESYSTEMWATCH_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <Windows.h>
#endif

	extern int listenforchanges(
#ifdef _WIN32
		LPCTSTR lpPathName
#endif
		, void(*callback)());

#ifdef __cplusplus
}
#endif

#endif