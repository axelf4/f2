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

#define DEFAULT_PORT 6622
#ifndef DEFAULT_BUFLEN
#define DEFAULT_BUFLEN 512
#endif

#define SOCK_ADDR_FAMILY(ptr)	((struct sockaddr *)(ptr))->sa_family
#define SOCK_ADDR_IN_FAMILY(sa)	((struct sockaddr_in *)(sa))->sin_family
#define SOCK_ADDR_IN_PORT(sa)	((struct sockaddr_in *)(sa))->sin_port
#define SOCK_ADDR_IN_ADDR(sa)	((struct sockaddr_in *)(sa))->sin_addr
#ifdef HAS_IPV6
#define SOCK_ADDR_IN6_PORT(sa)	((struct sockaddr_in6 *)(sa))->sin6_port
#define SOCK_ADDR_IN6_ADDR(sa)	((struct sockaddr_in6 *)(sa))->sin6_addr
#define SOCK_ADDR_EQ_ADDR(sa, sb) \
    ((SOCK_ADDR_FAMILY(sa) == AF_INET && SOCK_ADDR_FAMILY(sb) == AF_INET \
      && SOCK_ADDR_IN_ADDR(sa).s_addr == SOCK_ADDR_IN_ADDR(sb).s_addr) \
     || (SOCK_ADDR_FAMILY(sa) == AF_INET6 && SOCK_ADDR_FAMILY(sb) == AF_INET6 \
         && memcmp((char *) &(SOCK_ADDR_IN6_ADDR(sa)), \
                   (char *) &(SOCK_ADDR_IN6_ADDR(sb)), \
                   sizeof(SOCK_ADDR_IN6_ADDR(sa))) == 0))

#define SOCK_ADDR_EQ_PORT(sa, sb) \
    ((SOCK_ADDR_FAMILY(sa) == AF_INET && SOCK_ADDR_FAMILY(sb) == AF_INET \
      && SOCK_ADDR_IN_PORT(sa) == SOCK_ADDR_IN_PORT(sb)) \
     || (SOCK_ADDR_FAMILY(sa) == AF_INET6 && SOCK_ADDR_FAMILY(sb) == AF_INET6 \
         && SOCK_ADDR_IN6_PORT(sa) == SOCK_ADDR_IN6_PORT(sb)))
#else
#define SOCK_ADDR_EQ_ADDR(sa, sb) \
    (SOCK_ADDR_FAMILY(sa) == AF_INET && SOCK_ADDR_FAMILY(sb) == AF_INET \
     && SOCK_ADDR_IN_ADDR(sa).s_addr == SOCK_ADDR_IN_ADDR(sb).s_addr)

#define SOCK_ADDR_EQ_PORT(sa, sb) \
    (SOCK_ADDR_FAMILY(sa) == AF_INET && SOCK_ADDR_FAMILY(sb) == AF_INET \
     && SOCK_ADDR_IN_PORT(sa) == SOCK_ADDR_IN_PORT(sb))
#endif

	struct addr { const char *address; unsigned short port; };

	struct conn {
		struct sockaddr_in addr;
	};

	struct packet {
		/** The packet id. 1st byte internal information such as whether this is an ack packet. If this is an ack pakcet then the blob only contains the sequence. */
		unsigned int sequence;

		char *blob;
	};

	struct peer {
#ifdef _WIN32
		SOCKET socket;
#endif
		struct conn **connections;
		int numConnections;

		void(*accepted)(struct peer *);
	};

	// TODO implement support for BSD sockets and winsock2
	// TODO struct addr for inet addresses

	extern void net_initialize();

	extern struct peer * net_peer_bind(struct addr addr);

	/**
	@param address the address at which peers may connect to this peer */
	extern struct peer * net_peer_create(struct addr *address, unsigned short maxConnections);

	extern void net_peer_dispose(struct peer *peer);

	// TODO add milliseconds timeout
	extern void net_update(struct peer *peer);

	extern void net_send(struct peer *peer, const char *buf, int len);

	extern char * net_packet(int len);

#ifdef __cplusplus
}
#endif

#endif