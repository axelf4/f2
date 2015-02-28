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
	/** Compares the address families and network addresses for equality, returning non-zero in that case.
		@def SOCK_ADDR_EQ_ADDR(sa, sb)
		@param sa The first socket address of type @code struct sockaddr * @code to be compared
		@param sb The second socket address of type @code struct sockaddr * @code to be compared
		@return Zero if the arguments differ, otherwise non-zero
		@warning Evaluates the arguments multiple times */
#define SOCK_ADDR_EQ_ADDR(sa, sb) (((struct sockaddr *)(sa))->sa_family == AF_INET && ((struct sockaddr *)(sb))->sa_family == AF_INET && ((struct sockaddr_in *)(sa))->sin_addr.s_addr == ((struct sockaddr_in *)(sb))->sin_addr.s_addr)
	/** Compares the address families and ports for equality, returning non-zero in that case.
		@def SOCK_ADDR_EQ_PORT(sa, sb)
		@param sa The first socket address of type @code struct sockaddr * @code to be compared
		@param sb The second socket address of type @code struct sockaddr * @code to be compared
		@return Zero if the arguments differ, otherwise non-zero
		@warning Evaluates the arguments multiple times */
#define SOCK_ADDR_EQ_PORT(sa, sb) (((struct sockaddr *)(sa))->sa_family == AF_INET && ((struct sockaddr *)(sb))->sa_family == AF_INET && ((struct sockaddr_in *)(sa))->sin_port == ((struct sockaddr_in *)(sb))->sin_port)
#endif

#define NET_SEQNO_SIZE 2
	/** 256 - 3, one for zero, one for SYN packets and one for pings. */
#define NET_SEQNO_MAX ((1 << (NET_SEQNO_SIZE) * 8) - 3)
#define NET_PING_SEQNO (NET_SEQNO_MAX + 1)
#define NET_NAK_SEQNO (NET_SEQNO_MAX + 2)

#define NET_UNRELIABLE 0
#define NET_RELIABLE 1

	// struct addr { const char *address; unsigned short port; };

	struct sockaddr_in net_address(const char *address, unsigned int port);

	struct conn {
		struct sockaddr_in addr;
		int sentLengths[NET_SEQNO_MAX]; /** The lengths, in bytes, of the buffers in #sentBuffers. */
		unsigned char *sentBuffers[NET_SEQNO_MAX], /**< History buffer */
			missing[NET_SEQNO_MAX]; /**< Array of 1s and 0s. 0 is for packet at index (seqno - 1) has arrived. 1 is for waiting for packet. Initialized with zeros. */
		unsigned int lastSent, /**< The sequence number of the last sent packet (defaults to 0).*/
			lastReceived; /**< The sequence number of the last received packet (defaults to 0). */
	};

	struct peer {
#ifdef _WIN32
		SOCKET socket;
#endif
		struct conn **connections;
		unsigned int numConnections;

		void(*accept)(struct peer *, struct conn *);
	};

	// TODO implement support for BSD sockets and winsock2
	// TODO struct addr for inet addresses

	extern void net_initialize();

	extern void net_deinitialize();

	/**
	@param address the address at which peers may connect to this peer */
	extern struct peer * net_peer_create(struct sockaddr_in *recvaddr, unsigned short maxConnections);

	extern void net_peer_dispose(struct peer *peer);

	extern void net_update(struct peer *peer);

	/** Sends a packet to the specified remote end.
		@return -1 in case of an error, otherwise the number of sent bytes.
		@warning Make sure to leave 1 byte empty in \a buf and have \a len reflect that! */
	extern int net_send(struct peer *peer, unsigned char *buf, int len, struct sockaddr_in to, int flag);

	/** Receives a packet from an remote end.
		@param buflen the maximum number of bytes to read to the buffer
		@return -1 in case of an error, otherwise the number of bytes read, or 0 */
	extern int net_receive(struct peer *peer, unsigned char *buf, int buflen, struct sockaddr_in *from);

#ifdef __cplusplus
}
#endif

#endif