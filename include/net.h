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
#define SOCK_ADDR_EQ_ADDR(sa, sb) \
	((SOCK_ADDR_FAMILY(sa) == AF_INET && SOCK_ADDR_FAMILY(sb) == AF_INET \
	&& SOCK_ADDR_IN_ADDR(sa).s_addr == SOCK_ADDR_IN_ADDR(sb).s_addr) \
	|| (SOCK_ADDR_FAMILY(sa) == AF_INET6 && SOCK_ADDR_FAMILY(sb) == AF_INET6 \
	&& memcmp((char *) &((struct sockaddr_in6 *)(sa))->sin6_addr, \
		(char *) &((struct sockaddr_in6 *)(sa))->sin6_addr, \
		sizeof ((struct sockaddr_in6 *)(sa))->sin6_addr) == 0))

#define SOCK_ADDR_EQ_PORT(sa, sb) \
	((SOCK_ADDR_FAMILY(sa) == AF_INET && SOCK_ADDR_FAMILY(sb) == AF_INET \
	&& SOCK_ADDR_IN_PORT(sa) == SOCK_ADDR_IN_PORT(sb)) \
	|| (SOCK_ADDR_FAMILY(sa) == AF_INET6 && SOCK_ADDR_FAMILY(sb) == AF_INET6 \
	&& ((struct sockaddr_in6 *)(sa))->sin6_port == ((struct sockaddr_in6 *)(sb))->sin6_port))
#else
#define SOCK_ADDR_EQ_ADDR(sa, sb) \
	(SOCK_ADDR_FAMILY(sa) == AF_INET && SOCK_ADDR_FAMILY(sb) == AF_INET \
	&& SOCK_ADDR_IN_ADDR(sa).s_addr == SOCK_ADDR_IN_ADDR(sb).s_addr)

#define SOCK_ADDR_EQ_PORT(sa, sb) \
	(SOCK_ADDR_FAMILY(sa) == AF_INET && SOCK_ADDR_FAMILY(sb) == AF_INET \
	&& SOCK_ADDR_IN_PORT(sa) == SOCK_ADDR_IN_PORT(sb))
#endif

	/** 255 - 1 */
#define NET_MAX_TO_BE_ACKNOWLEDGED 254

	// struct addr { const char *address; unsigned short port; };

	struct sockaddr_in net_address(const char *address, unsigned int port);

	struct conn {
		struct sockaddr_in addr;
		// History buffer
		char *sentBuffers[NET_MAX_TO_BE_ACKNOWLEDGED];
		int sentLengths[NET_MAX_TO_BE_ACKNOWLEDGED];
		char sentCounter;

		/** The last sequence number received. */
		unsigned char lastReceived;
		/** Array of 1s and 0s. 1 is for packet at index (seqno - 1) has arrived. 0 is for waiting for packet. Initialized with ones. */
		char receivedSeqnos[NET_MAX_TO_BE_ACKNOWLEDGED];
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

		void(*accept)(struct peer *, struct conn *);
	};

	// TODO implement support for BSD sockets and winsock2
	// TODO struct addr for inet addresses

	extern void net_initialize();

	/**
	@param address the address at which peers may connect to this peer */
	extern struct peer * net_peer_create(struct sockaddr_in *recvaddr, unsigned short maxConnections);

	extern void net_peer_dispose(struct peer *peer);

	// TODO add milliseconds timeout
	extern void net_update(struct peer *peer);

	/** Sends a packet to the specified remote end.
		@warning Make sure to leave 1 byte empty in \a buf and have \a len reflect that! */
	extern void net_send(struct peer *peer, char *buf, int len, struct sockaddr_in to, int reliable);

	/** Receives a packet from an remote end.
		@param buflen the maximum number of bytes to read to the buffer
		@return the number of bytes read, or 0 */
	extern int net_receive(struct peer *peer, char *buf, int buflen, struct sockaddr_in *from);

#ifdef __cplusplus
}
#endif

#endif