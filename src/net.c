#include "net.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "timer.h"
#include <fcntl.h>

static struct conn * add_connection(struct peer *peer, struct sockaddr address) {
	struct conn *connection = malloc(sizeof(struct conn));
	connection->address = address;
	connection->lastSent = connection->lastReceived = 0;
	connection->lastSendTime = connection->lastReceiveTime = 0;
	for (unsigned int i = 0; i < NET_SEQNO_MAX; i++) {
		connection->sentBuffers[i] = 0;
		connection->missing[i] = 0;
	}
	connection->data = 0;

	peer->connections[peer->numConnections++] = connection;

	return connection;
}

int net_initialize() {
#ifdef _WIN32
	WSADATA wsaData;
	return WSAStartup(MAKEWORD(2, 2), &wsaData);// Initialize Winsock
#endif
}

void net_deinitialize() {
#ifdef _WIN32
	WSACleanup();
#endif
}

struct peer * net_peer_create(struct sockaddr *recvaddr, unsigned short maxConnections) {
	struct peer *peer = (struct peer *) malloc(sizeof(struct peer));
	peer->connections = malloc(sizeof(struct conn *) * maxConnections);
	peer->numConnections = 0;
	peer->accept = 0;
	peer->disconnect = 0;

	// Create a socket
#ifdef _WIN32
	SOCKET
#else
	int
#endif
		sockfd;
	if ((peer->socket = sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) goto error;
	// Make the socket non-blocking
#ifdef _WIN32
	u_long mode = 1;
	if (ioctlsocket(sockfd, FIONBIO, &mode) == SOCKET_ERROR)
#else
	if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1)
#endif
		goto error;

	// Optionally bind the socket
	if (recvaddr != 0) {
		// ((struct sockaddr_in *)recvaddr)->sin_addr.s_addr = INADDR_ANY;
		if (bind(sockfd, recvaddr, sizeof(struct sockaddr_in)) != 0) goto error;
	}
	return peer;

error:
	net_peer_dispose(peer);
	return 0;
}

void net_peer_dispose(struct peer *peer) {
#ifdef _WIN32
	if (peer->socket != INVALID_SOCKET) closesocket
#else
	if (peer->socket != -1) close
#endif
		(peer->socket);

	// Free up the connections
	for (unsigned int i = 0; i < peer->numConnections; i++) {
		struct conn *connection = peer->connections[i];
		free(connection->data);
		free(connection);
	}
	free(peer->connections);
	free(peer);
}

void net_update(struct peer *peer) {
	for (unsigned int i = 0; i < peer->numConnections; i++) {
		struct conn *connection = peer->connections[i];

		for (unsigned int j = 0; j < NET_SEQNO_MAX; j++) {
			if (connection->missing[j]) {
				int naklen = NET_SEQNO_SIZE * 2;
				unsigned char nak[NET_SEQNO_SIZE * 2];
				for (int i = 0; i < NET_SEQNO_SIZE; i++) nak[i] = (j + 1) >> (NET_SEQNO_SIZE - i - 1) * 8;
				printf("Sending a request to resend packet %d\n", j + 1);
				for (int i = 0; i < NET_SEQNO_SIZE; i++) nak[naklen - 1 - i] = NET_NAK_SEQNO >> i * 8;
				net_send(peer, nak, naklen, &connection->address, 0);
			}
		}

		double now = time_get();
		if (now - connection->lastSendTime > NET_PING_INTERVAL ||
			(connection->lastReceiveTime != 0 && now - connection->lastReceiveTime > NET_PING_INTERVAL)) {
			int pinglen = NET_SEQNO_SIZE * 2;
			unsigned char ping[NET_SEQNO_SIZE * 2];
			for (int i = 0; i < NET_SEQNO_SIZE; i++) ping[i] = connection->lastSent >> (NET_SEQNO_SIZE - i - 1) * 8;
			for (int i = 0; i < NET_SEQNO_SIZE; i++) ping[pinglen - 1 - i] = NET_PING_SEQNO >> i * 8;
			net_send(peer, ping, pinglen, &connection->address, 0); // Send ping
		}

		if (connection->lastReceiveTime != 0 && now - connection->lastReceiveTime > 20000) {
			// TODO drop client
			if (peer->disconnect != 0) peer->disconnect(peer, connection);
		}
	}
}

int net_send(struct peer *peer, unsigned char *buf, int len, const struct sockaddr *to, int flag) {
	if (flag & NET_UNRELIABLE) {
		for (int i = 0; i < NET_SEQNO_SIZE; i++) buf[len - 1 - i] = 0;
	}
	else if (flag & NET_RELIABLE) {
		struct conn *connection = 0;
		for (unsigned int i = 0; i < peer->numConnections; i++) {
			struct conn *other = peer->connections[i];
			struct sockaddr connAddr = other->address;
			if (SOCK_ADDR_EQ_ADDR(to, &connAddr) && SOCK_ADDR_EQ_PORT(to, &connAddr)) {
				connection = other;
				break;
			}
		}
		if (connection == 0) connection = add_connection(peer, *to);
		connection->lastSendTime = time_get();

		// if (++connection->lastSent > NET_SEQNO_MAX)	connection->lastSent = 1;
		connection->lastSent = connection->lastSent % NET_SEQNO_MAX + 1;
		for (int i = 0; i < NET_SEQNO_SIZE; i++) buf[len - 1 - i] = connection->lastSent >> i * 8;
		// buf[len - 1] = connection->lastSent;

		unsigned char **historyBuf = connection->sentBuffers + connection->lastSent - 1;
		if (*historyBuf != 0) free(*historyBuf); // If there was an old buffer at the index: free it
		*historyBuf = buf;
		connection->sentLengths[connection->lastSent - 1] = len;
	}

	return sendto(peer->socket, buf, len, 0, to, sizeof(struct sockaddr_in));
}

int net_receive(struct peer *peer, unsigned char *buf, int buflen, struct sockaddr *from) {
beginning:; // If received a packet used internally: don't return, but skip it
	socklen_t fromlen = sizeof(struct sockaddr_in), result;
	if ((result = recvfrom(peer->socket, buf, buflen, 0, from, &fromlen)) == -1) {
		int error = WSAGetLastError();
#ifdef _WIN32
		if (error == WSAEWOULDBLOCK || error == WSAECONNRESET)
#else
		if (error == EWOULDBLOCK || error == ECONNRESET)
#endif
			return 0;
	}
	else if (result > 0) {
		if (rand() % 2) goto beginning; // Simulate packet drop locally

		// Find out if the packet forms a new connection
		struct conn *connection = 0;
		for (unsigned int i = 0; i < peer->numConnections; i++) {
			struct conn *other = peer->connections[i];
			struct sockaddr address = other->address;
			if (SOCK_ADDR_EQ_ADDR(from, &address) && SOCK_ADDR_EQ_PORT(from, &address)) {
				connection = other; // The origin of the packet and the connection's address match: existing connection
				break;
			}
		}
		if (!connection) {
			connection = add_connection(peer, *from); // First time receiving from the remote end's address; create new connection
			if (peer->accept != 0) peer->accept(peer, connection); // Alert the application about it
		}
		connection->lastReceiveTime = time_get();

		// unsigned char seqno = *(buf + result - NET_SEQNO_SIZE);
		unsigned int seqno = 0; // The sequence number is located last in the buffer
		for (int i = 0; i < NET_SEQNO_SIZE; i++) seqno |= buf[result - 1 - i] << i * 8;
		if (seqno == NET_NAK_SEQNO) {
			unsigned int no = 0;
			for (int i = 0; i < NET_SEQNO_SIZE; i++) no |= buf[i] << (NET_SEQNO_SIZE - i - 1) * 8;
			printf("The remote end has sent a request to resend packet %d.\n", no);
			net_send(peer, connection->sentBuffers[no - 1], connection->sentLengths[no - 1], from, 0); // The remote end requested a resend of the packet with the id *buf
			goto beginning; // Don't return the internal packet!
		}
		else if (seqno) { // A ping or reliable packet
			unsigned int no = 0;
			// If a ping; use the last sent packet's number, else use the received packet's
			if (seqno == NET_PING_SEQNO) {
				for (int i = 0; i < NET_SEQNO_SIZE; i++) no |= buf[i] >> (NET_SEQNO_SIZE - i - 1) * 8;
			}
			else no = seqno;

			// If the packet in question is more recent than the last received:
			if ((no > connection->lastReceived && no - connection->lastReceived <= NET_SEQNO_MAX / 2) ||
				(no < connection->lastReceived && connection->lastReceived - no > NET_SEQNO_MAX / 2)) {
				// Mark all packets with numbers between the last received's and the received one's as missing
				unsigned int numToComp = no % NET_SEQNO_MAX + (seqno == NET_PING_SEQNO);
				if (no == NET_SEQNO_MAX) numToComp = seqno == NET_PING_SEQNO ? 1 : NET_SEQNO_MAX;
				for (unsigned int i = (connection->lastReceived) % NET_SEQNO_MAX + 1; i != numToComp; i = (i) % NET_SEQNO_MAX + 1) {
					connection->missing[i - 1] = 1;
				}
				connection->lastReceived = no; // Update the last received sequence number
			}
			else if (!connection->missing[seqno - 1]) goto beginning; // The packet has already arrived
			if (seqno == NET_PING_SEQNO) goto beginning; // The packet was internally used as a ping
			else connection->missing[seqno - 1] = 0; // Mark the packet as received
		}

		/*printf("Sequence number is %u/", seqno);
		for (int i = NET_SEQNO_SIZE * 8 - 1; i >= 0; i--) putchar((seqno & (1 << i)) ? '1' : '0');
		putchar('\n');*/
	}
	return result;
}
