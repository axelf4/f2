/** Functions for reliably sending UDP packets.
	@file net.h */

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
#endif

#define DEFAULT_PORT 6622
#ifndef DEFAULT_BUFLEN
#define DEFAULT_BUFLEN 512
#endif

#ifdef HAS_IPV6
#define SOCK_ADDR_EQ_ADDR(sa, sb) \
	(((struct sockaddr *)(sa))->sa_family == AF_INET && ((struct sockaddr *)(sb))->sa_family == AF_INET \
	&& ((struct sockaddr_in *)(sa))->sin_addr.s_addr == ((struct sockaddr_in *)(sb))->sin_addr.s_addr) \
	|| (((struct sockaddr *)(sa))->sa_family == AF_INET6 && ((struct sockaddr *)(sb))->sa_family == AF_INET6 \
	&& memcmp((char *) &((struct sockaddr_in6 *)(sa))->sin6_addr, (char *) &((struct sockaddr_in6 *)(sa))->sin6_addr, sizeof ((struct sockaddr_in6 *)(sa))->sin6_addr) == 0))
#define SOCK_ADDR_EQ_PORT(sa, sb) \
	((((struct sockaddr *)(sa))->sa_family == AF_INET && ((struct sockaddr *)(sb))->sa_family == AF_INET \
	&& ((struct sockaddr_in *)(sa))->sin_port == ((struct sockaddr_in *)(sb))->sin_port) \
	|| (((struct sockaddr *)(sa))->sa_family == AF_INET6 && ((struct sockaddr *)(sb))->sa_family == AF_INET6 \
	&& ((struct sockaddr_in6 *)(sa))->sin6_port == ((struct sockaddr_in6 *)(sb))->sin6_port))
#else
	/** Compares the address families and network addresses for equality, and returns non-zero in that case.
		@def SOCK_ADDR_EQ_ADDR(sa, sb)
		@param sa The first socket address of type @code struct sockaddr * @code to be compared
		@param sb The second socket address of type @code struct sockaddr * @code to be compared
		@return Zero if the arguments differ, otherwise non-zero
		@warning Evaluates the arguments multiple times */
#define SOCK_ADDR_EQ_ADDR(sa, sb) (((struct sockaddr *)(sa))->sa_family == AF_INET && ((struct sockaddr *)(sb))->sa_family == AF_INET && ((struct sockaddr_in *)(sa))->sin_addr.s_addr == ((struct sockaddr_in *)(sb))->sin_addr.s_addr)
	/** Compares the address families and ports for equality, and returns non-zero in that case.
		@def SOCK_ADDR_EQ_PORT(sa, sb)
		@param sa The first socket address of type @code struct sockaddr * @code to be compared
		@param sb The second socket address of type @code struct sockaddr * @code to be compared
		@return Zero if the arguments differ, otherwise non-zero
		@warning Evaluates the arguments multiple times */
#define SOCK_ADDR_EQ_PORT(sa, sb) (((struct sockaddr *)(sa))->sa_family == AF_INET && ((struct sockaddr *)(sb))->sa_family == AF_INET && ((struct sockaddr_in *)(sa))->sin_port == ((struct sockaddr_in *)(sb))->sin_port)
#endif

	/** 255 - 2, one for SYN packets and one for pings */
#define NET_MAX_TO_BE_SYNCHRONIZED 0xFD

	// struct addr { const char *address; unsigned short port; };

	struct sockaddr_in net_address(const char *address, unsigned int port);

	struct conn {
		struct sockaddr_in addr;
		int sentLengths[NET_MAX_TO_BE_SYNCHRONIZED]; /** The lengths, in bytes, of the buffers in #sentBuffers. */
		unsigned char *sentBuffers[NET_MAX_TO_BE_SYNCHRONIZED], /**< History buffer */
			lastSent, /**< The sequence number of the last sent packet (defaults to 0).*/
			lastReceived, /**< The sequence number of the last received packet (defaults to 0). */
			receivedSeqnos[NET_MAX_TO_BE_SYNCHRONIZED]; /**< Array of 1s and 0s. 1 is for packet at index (seqno - 1) has arrived. 0 is for waiting for packet. Initialized with ones. */
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