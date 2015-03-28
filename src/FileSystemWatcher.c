#include "FileSystemWatcher.h"

#ifdef _WIN32
#include <tchar.h>
#endif

struct FileSystemWatcher *create_FileSystemWatcher() {
	struct FileSystemWatcher *watcher = malloc(sizeof(struct FileSystemWatcher));
	if (watcher == 0) return 0;

#ifndef _WIN32
	if ((watcher->fd = inotify_init()) < 0) {
		free(watcher);
		return 0;
	}
#endif

	watcher->count = 0;
	watcher->handles = 0;
	return watcher;
}

void destroy_FileSystemWatcher(struct FileSystemWatcher *watcher) {
#ifdef _WIN32
	for (int i = 0; i < watcher->count; i++) {
		FindCloseChangeNotification(watcher->handles[i]);
	}
	free(watcher->handles);
#else
	if (watcher->wd != -1) inotify_rm_watch(watcher->fd, watcher->wd);
	close(watcher->fd);
#endif
	free(watcher);
}

int add_FileSystemWatcherHandle(struct FileSystemWatcher *watcher, FileSystemWatcherHandle handle) {
	FileSystemWatcherHandle *tmp = realloc(watcher->handles, sizeof(FileSystemWatcherHandle) * ++watcher->count);
	if (tmp == 0) {
		return 0;
	}
	(watcher->handles = tmp)[watcher->count - 1] = handle;
	return 1;
}

int add_FileSystemWatcherTarget(struct FileSystemWatcher *watcher,
#ifdef _WIN32
	LPTSTR
#else
	const char *
#endif
	dir, enum FileNotifyFlag flag) {
	FileSystemWatcherHandle handle;
#ifdef _WIN32
	// inotify doesn't support watching subtree's so it's a no-no
	if ((handle = FindFirstChangeNotification(dir, FALSE, flag)) == INVALID_HANDLE_VALUE) {
#else
	if ((handle = inotify_add_watch(watcher->fd, dir, watcher->flag = flag)) == -1) {
#endif
		return 0;
	}
	return add_FileSystemWatcherHandle(watcher, handle);
}

int wait_FileSystemWatcher(struct FileSystemWatcher *watcher) {
#ifdef _WIN32
	DWORD waitStatus = WaitForMultipleObjects(watcher->count, watcher->handles, FALSE, INFINITE);
	if (waitStatus >= WAIT_OBJECT_0 && waitStatus < WAIT_OBJECT_0 + watcher->count) {
		DWORD i = waitStatus - WAIT_OBJECT_0;
		/*if (*/ FindNextChangeNotification(watcher->handles[i]); /* == FALSE) {	return -1; } */
		return i;
	}
#else
	int buflen = 1024 * (sizeof(struct inotify_event) + 16 );
	char buffer[buflen];
	int i, length = read(fd, buffer, buflen);
	if (length < 0) return 0;

	while (i < length) {
		struct inotify_event *event = (struct inotify_event *) &buffer[i];
		if (event->len) {
			if (event->mask & IN_CREATE) {
				if (event->mask & IN_ISDIR) {
					printf("The directory %s was created.\n", event->name);
				}
				else {
					printf("The file %s was created.\n", event->name);
				}
			}
			else if (event->mask & IN_DELETE) {
				if (event->mask & IN_ISDIR) {
					printf("The directory %s was deleted.\n", event->name);
				}
				else {
					printf("The file %s was deleted.\n", event->name);
				}
			}
			else if (event->mask & IN_MODIFY) {
				if (event->mask & IN_ISDIR) {
					printf("The directory %s was modified.\n", event->name);
				}
				else {
					printf("The file %s was modified.\n", event->name);
				}
			}
		}
		i += sizeof(struct inotify_event) + event->len;
	}
#endif
	return 1;
}