#ifndef NET_H
#define NET_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32	
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
	// #pragma comment (lib, "Mswsock.lib")
	// #pragma comment (lib, "AdvApi32.lib")

	// TODO put socket in non-blocking mode and use select
#endif

#define DEFAULT_PORT "6622"
#ifndef DEFAULT_BUFLEN
#define DEFAULT_BUFLEN 512
#endif

	struct addr { const char *address, *port; };

	struct HOST {
#ifdef _WIN32
		SOCKET s;
#endif
		// Send buffer
		char buffer[DEFAULT_BUFLEN];
		int position;
	};

	// TODO implement support for BSD sockets and winsock2
	// TODO struct addr for inet addresses

	extern void net_initialize();

	extern struct HOST * net_host_bind(struct addr addr);

	// OLD CODE BELOW

	extern void net_client_socket(const char *addr, const char *port);

	extern void net_server_socket();

	extern int net_client_socket2(const char *address);

	extern int net_server_socket2();

#ifdef __cplusplus
}
#endif

#endif