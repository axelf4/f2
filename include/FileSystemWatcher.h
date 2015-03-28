#ifndef FILE_SYSTEM_WATCHER_H
#define FILE_SYSTEM_WATCHER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/inotify.h>
#endif

	enum FileNotifyFlag {
#ifdef _WIN32
		FILE_NOTIFY_ATTRIB = FILE_NOTIFY_CHANGE_ATTRIBUTES,
		FILE_NOTIFY_MODIFY = FILE_NOTIFY_CHANGE_LAST_WRITE
#else
		FILE_NOTIFY_ATTRIB = IN_ATTRIB,
		FILE_NOTIFY_MODIFY = IN_MODIFY
#endif
	};

	typedef
#ifdef _WIN32
		HANDLE
#else
		int /* Watch descriptor */
#endif
		FileSystemWatcherHandle;

	struct FileSystemWatcher {
#ifndef _WIN32
		int fd;
		int
#else
		DWORD
#endif
			count;
		FileSystemWatcherHandle *handles;
	};

	extern struct FileSystemWatcher *create_FileSystemWatcher();

	extern void destroy_FileSystemWatcher(struct FileSystemWatcher *watcher);

	extern int add_FileSystemWatcherHandle(struct FileSystemWatcher *watcher, FileSystemWatcherHandle handle);

	extern int add_FileSystemWatcherTarget(struct FileSystemWatcher *watcher,
#ifdef _WIN32
		LPTSTR
#else
		const char *
#endif
		dir, enum FileNotifyFlag flag);

	extern int wait_FileSystemWatcher(struct FileSystemWatcher *watcher);

#ifdef __cplusplus
}
#endif

#endif