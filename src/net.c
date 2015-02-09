#include "net.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static struct conn * add_connection(struct peer *peer, struct sockaddr_in addr) {
	struct conn *connection = malloc(sizeof(struct conn));
	connection->addr = addr;
	connection->lastSent = 0;
	connection->lastReceived = 0;
	for (unsigned int i = 0; i < NET_SEQNO_MAX; i++) {
		connection->sentBuffers[i] = 0;
		connection->receivedSeqnos[i] = 1; // TODO rename and zero initialize
	}

	peer->connections[peer->numConnections++] = connection;

	return connection;
}

void net_initialize() {
#ifdef _WIN32
	WSADATA wsaData;
	// Initialize Winsock
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		fprintf(stderr, "WSAStartup failed with error: %d\n", result);
	}
#endif
}

struct sockaddr_in net_address(const char *address, unsigned int port) {
	struct sockaddr_in retval = { .sin_family = AF_INET, .sin_port = htons(port), .sin_addr.s_addr = inet_addr(address) };
	return retval;
}

struct peer * net_peer_create(struct sockaddr_in *recvaddr, unsigned short maxConnections) {
	SOCKET Socket;
	if ((Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
		printf("socket failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 0;
	}
	u_long mode = 1;
	if (ioctlsocket(Socket, FIONBIO, &mode) == SOCKET_ERROR) {
		printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
	}

	if (recvaddr != 0) {
		// struct sockaddr_in RecvAddr;
		// RecvAddr.sin_family = AF_INET;
		// RecvAddr.sin_port = htons(6622); // TODO the address field for is not an ushort
		// RecvAddr.sin_port = htons(address->port);
		// RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		recvaddr->sin_addr.s_addr = INADDR_ANY;

		int i = bind(Socket, (struct sockaddr *) recvaddr, sizeof(struct sockaddr_in));
		if (i != 0) {
			printf("bind failed with error %d\n", WSAGetLastError());
			return 0;
		}
	}

	struct peer *peer = (struct peer *) malloc(sizeof(struct peer));
	peer->socket = Socket;
	peer->connections = malloc(sizeof(struct conn *) * maxConnections);
	peer->numConnections = 0;
	return peer;
}

void net_peer_dispose(struct peer *peer) {
	int iResult = closesocket(peer->socket);
	if (iResult == SOCKET_ERROR) {
		wprintf(L"closesocket failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return;
	}

	// Free up the connections
	for (unsigned int i = 0; i < peer->numConnections; i++) {
		free(peer->connections[i]);
	}
	free(peer->connections);

	WSACleanup();

	free(peer);
}

void net_update(struct peer *peer) {
	for (unsigned int i = 0; i < peer->numConnections; i++) {
		struct conn *connection = peer->connections[i];

		for (unsigned int j = 0; j < NET_SEQNO_MAX; j++) {
			if (!connection->receivedSeqnos[j]) {
				// char syn[] = { j + 1, 0xFF };
				int naklen = NET_SEQNO_SIZE * 2;
				char *nak = calloc(naklen, sizeof(char));
				// *syn = j + 1;
				for (int i = 0; i < NET_SEQNO_SIZE; i++) nak[i] = (j + 1) >> (NET_SEQNO_SIZE - i - 1) * 8;
				printf("Sending a request to resend packet %d\n", j + 1);
				for (int i = 0; i < NET_SEQNO_SIZE; i++) nak[naklen - 1 - i] = NET_NAK_SEQNO >> i * 8; // syn[synlen - 1] = NET_NAK_SEQNO;
				net_send(peer, nak, naklen, connection->addr, 2);
				free(nak);
			}
		}

		// TODO only ping if time since last receive has exceeded a certain threshold
		// Send ping
		// char buf[] = { connection->lastSent, 0xFE };
		int buflen = NET_SEQNO_SIZE * 2;
		char *buf = calloc(buflen, sizeof(char));
		// *buf = connection->lastSent;
		for (int i = 0; i < NET_SEQNO_SIZE; i++) buf[i] = connection->lastSent >> (NET_SEQNO_SIZE - i - 1) * 8;
		for (int i = 0; i < NET_SEQNO_SIZE; i++) buf[buflen - 1 - i] = NET_PING_SEQNO >> i * 8; // buf[buflen - 1] = NET_PING_SEQNO;
		net_send(peer, buf, buflen, connection->addr, 2);
		free(buf);
	}
}

// TODO make return error value
void net_send(struct peer *peer, unsigned char *buf, int len, struct sockaddr_in to, int reliable) {
	// printf("sending bytes\n");
	// struct sockaddr_in to = { .sin_family = AF_INET, .sin_port = htons(DEFAULT_PORT), .sin_addr.s_addr = inet_addr("127.0.0.1") };
	// int tolen = sizeof(to);

	if (!reliable) {
		for (int i = 1; i <= NET_SEQNO_SIZE; i++) buf[len - i] = 0;
	}
	else if (reliable != 2) {
		struct conn *connection = 0;
		for (unsigned int i = 0; i < peer->numConnections; i++) {
			struct conn *other = peer->connections[i];
			struct sockaddr_in connAddr = other->addr;
			if (SOCK_ADDR_EQ_ADDR(&to, &connAddr) && SOCK_ADDR_EQ_PORT(&to, &connAddr)) {
				connection = other;
				break;
			}
		}
		if (connection == 0) connection = add_connection(peer, to);

		// if (++connection->lastSent > NET_SEQNO_MAX)	connection->lastSent = 1;
		connection->lastSent = connection->lastSent % NET_SEQNO_MAX + 1;
		for (int i = 0; i < NET_SEQNO_SIZE; i++) buf[len - 1 - i] = connection->lastSent >> i * 8;
		// buf[len - 1] = connection->lastSent;

		char **historyBuf = connection->sentBuffers + connection->lastSent - 1;
		if (*historyBuf != 0) free(*historyBuf); // If there was an old buffer at the index; free it
		*historyBuf = buf;
		connection->sentLengths[connection->lastSent - 1] = len;
	}

	int result = sendto(peer->socket, buf, len, 0, (struct sockaddr *)&to, sizeof(struct sockaddr_in));
	if (result == SOCKET_ERROR) {
		printf("sendto failed with error: %d\n", WSAGetLastError());
		closesocket(peer->socket);
		WSACleanup();
		return;
	}
}

int net_receive(struct peer *peer, unsigned char *buf, int buflen, struct sockaddr_in *from) {
	// TODO first write this berkeley code then optimize for windows with WSA*.
beginning:; // If received a packet used internally: don't return, but skip it
	int fromlen = sizeof(struct sockaddr_in), result;
	if ((result = recvfrom(peer->socket, buf, buflen, 0, (struct sockaddr *)from, &fromlen)) == SOCKET_ERROR) {
		int error;
		if ((error = WSAGetLastError()) != WSAEWOULDBLOCK)
			printf("recvfrom failed with error %d\n", error);
	}
	else if (result > 0) {
		if (rand() % 2) goto beginning; // Simulate packet drop locally

		// Find out if the packet forms a new connection
		struct conn *connection = 0;
		for (unsigned int i = 0; i < peer->numConnections; i++) {
			struct conn *other = peer->connections[i];
			struct sockaddr_in address = other->addr;
			if (SOCK_ADDR_EQ_ADDR(from, &address) && SOCK_ADDR_EQ_PORT(from, &address)) {
				connection = other; // The origin of the packet and the connection's address match: existing connection
				break;
			}
		}
		if (!connection) {
			connection = add_connection(peer, *from); // First time receiving from the remote end's address; create new connection
			if (peer->accept != 0) peer->accept(peer, connection); // Alert the application about it
		}

		// unsigned char seqno = *(buf + result - NET_SEQNO_SIZE); // The sequence number is located last in the buffer
		unsigned int seqno = 0;
		for (int i = 0; i < NET_SEQNO_SIZE; i++) seqno |= buf[result - 1 - i] << i * 8;
		if (seqno == NET_NAK_SEQNO) {
			unsigned int no = 0;
			for (int i = 0; i < NET_SEQNO_SIZE; i++) no |= buf[i] << (NET_SEQNO_SIZE - i - 1) * 8;
			printf("The remote end has sent a request to resend packet %d.\n", no);
			net_send(peer, connection->sentBuffers[no - 1], connection->sentLengths[no - 1], *from, 2); // The remote end requested a resend of the packet with the id *buf
			goto beginning; // Don't return the internal packet!
		}
		else if (seqno) { // A ping or reliable packet
			unsigned int no = 0;
			// If a ping; use the last sent packet's number, else use the received packet's
			if (seqno == NET_PING_SEQNO) {
				for (int i = 0; i < NET_SEQNO_SIZE; i++) {
					no |= buf[i] >> (NET_SEQNO_SIZE - i - 1) * 8;
				}
			}
			else {
				no = seqno;
			}

			// If the packet in question is more recent than the last received:
			if ((no > connection->lastReceived && no - connection->lastReceived <= NET_SEQNO_MAX / 2) ||
				(no < connection->lastReceived && connection->lastReceived - no > NET_SEQNO_MAX / 2)) {
				// Mark all packets with numbers between the last received's and the received one's as missing
				unsigned int i = (connection->lastReceived) % NET_SEQNO_MAX + 1,
					numToComp = no % NET_SEQNO_MAX + (seqno == NET_PING_SEQNO);
				if (no == NET_SEQNO_MAX) numToComp = seqno == NET_PING_SEQNO ? 1 : NET_SEQNO_MAX;
				for (; i != numToComp; i = (i) % NET_SEQNO_MAX + 1) {
					connection->receivedSeqnos[i - 1] = 0;
				}
				connection->lastReceived = no; // Update the last received sequence number
			}
			else if (connection->receivedSeqnos[seqno - 1]) goto beginning; // The packet has already arrived
			if (seqno == NET_PING_SEQNO) goto beginning; // The packet was internally used as a ping
			else connection->receivedSeqnos[seqno - 1] = 1; // Mark the packet as received
		}

		printf("Sequence number is %u/", seqno);
		for (int i = NET_SEQNO_SIZE * 8 - 1; i >= 0; i--) putchar((seqno & (1 << i)) ? '1' : '0');
		putchar('\n');
	}
	return result;
}