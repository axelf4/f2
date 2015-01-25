#include "net.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct conn * add_connection(struct peer *peer, struct sockaddr_in addr) {
	struct conn *connection = malloc(sizeof(struct conn));
	connection->addr = addr;
	connection->sentCounter = 0;
	connection->lastReceived = 0; // NET_MAX_TO_BE_ACKNOWLEDGED;
	for (unsigned int i = 0; i < NET_MAX_TO_BE_ACKNOWLEDGED; i++) {
		connection->sentBuffers[i] = 0;
		connection->receivedSeqnos[i] = 1;
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
	int iResult;

	SOCKET Socket;
	// IPPROTO_UDP
	if ((Socket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
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

		iResult = bind(Socket, (struct sockaddr *) recvaddr, sizeof(struct sockaddr_in));
		if (iResult != 0) {
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
	for (int i = 0; i < peer->numConnections; i++) {
		free(peer->connections[i]);
	}
	free(peer->connections);

	WSACleanup();

	free(peer);
}

void net_update(struct peer *peer) {
	/*struct conn *conn = malloc(sizeof(struct conn));
	conn->socket = ClientSocket;
	peer->connections[peer->numConnections++] = conn;*/
	for (unsigned int i = 0; i < peer->numConnections; i++) {
		struct conn *connection = peer->connections[i];

		for (unsigned int j = 0; j < NET_MAX_TO_BE_ACKNOWLEDGED; j++) {
			if (!connection->receivedSeqnos[j]) {
				char syn[] = { j + 1, 0xFF };
				net_send(peer, syn, 2, connection->addr, 2);
			}
		}

		// Send ping
		char buf[] = { connection->sentCounter, 0xFE };
		net_send(peer, buf, 2, connection->addr, 2);
	}
}

// TODO make return error value
void net_send(struct peer *peer, char *buf, int len, struct sockaddr_in to, int reliable) {
	// printf("sending bytes\n");
	// struct sockaddr_in to = { .sin_family = AF_INET, .sin_port = htons(DEFAULT_PORT), .sin_addr.s_addr = inet_addr("127.0.0.1") };
	// int tolen = sizeof(to);

	struct conn *connection = 0;
	for (int i = 0; i < peer->numConnections; i++) {
		struct conn *other = peer->connections[i];
		struct sockaddr_in connAddr = other->addr;
		if (SOCK_ADDR_EQ_ADDR(&to, &connAddr) && SOCK_ADDR_EQ_PORT(&to, &connAddr)) {
			connection = other;
			break;
		}
	}
	if (!reliable) {
		buf[len - 1] = 0;
	}
	else if (reliable != 2) {
		if (connection == 0) connection = add_connection(peer, to);
		if (connection->sentCounter++ >= NET_MAX_TO_BE_ACKNOWLEDGED) connection->sentCounter = 1;
		buf[len - 1] = connection->sentCounter;
		char **historyBuf = connection->sentBuffers + connection->sentCounter - 1;
		if (*historyBuf != 0) free(*historyBuf); // If there was an old buffer at the index; free it
		*historyBuf = buf;
		connection->sentLengths[connection->sentCounter - 1] = len;
	}

	int result = sendto(peer->socket, buf, len, 0, (struct sockaddr *)&to, sizeof(struct sockaddr_in));
	if (result == SOCKET_ERROR) {
		printf("sendto failed with error: %d\n", WSAGetLastError());
		closesocket(peer->socket);
		WSACleanup();
		return;
	}
}

int net_receive(struct peer *peer, char *buf, int buflen, struct sockaddr_in *from) {
	// TODO first write this berkeley code then optimize for windows with WSA*.
beginning:; // If received a packet used internally: don't return but skip
	int fromlen = sizeof(struct sockaddr_in), result;
	if ((result = recvfrom(peer->socket, buf, buflen, 0, (struct sockaddr *)from, &fromlen)) == SOCKET_ERROR) {
		int error;
		if ((error = WSAGetLastError()) != WSAEWOULDBLOCK)
			printf("recvfrom failed with error %d\n", error);
	}
	else if (result > 0) {
		if (rand() % 2) goto beginning; // Simulate packet drop locally
		struct conn *connection = 0;
		for (int i = 0; i < peer->numConnections; i++) {
			struct conn *other = peer->connections[i];
			struct sockaddr_in connAddr = other->addr;
			// if (memcmp(&from, &connAddr, sizeof(struct sockaddr_in)) == 0) {
			if (SOCK_ADDR_EQ_ADDR(from, &connAddr) && SOCK_ADDR_EQ_PORT(from, &connAddr)) {
				connection = other;
				break;
			}
		}
		if (!connection) {
			connection = add_connection(peer, *from); // First time receiving from the remote end's address
			printf("New connection.\n");

			peer->accept(peer, connection);
		}

		unsigned char seqno = *(buf + result - 1);
		if (seqno == 0xFF) {
			// TODO make 0xFF resend a packet instead of declaring it finished
			printf("The remote end has sent a request to resend packet %d.\n", *buf);
			net_send(peer, connection->sentBuffers[*buf - 1], connection->sentLengths[*buf - 1], *from, 2);
			goto beginning; // Don't return the internal packet!
		}
		else if (seqno) { // A ping or reliable packet
			unsigned char no = seqno == 0xFE ? *buf + 1 : seqno; // If a ping; use the last packet sent's number, else use the received packet's
			// If the packet in question is newer than the last received:
			if (no > connection->lastReceived) {
				// Mark all packets with numbers between the last received's and the received's as missing
				for (unsigned int i = connection->lastReceived + 1; i < no; i = (i + 1) % NET_MAX_TO_BE_ACKNOWLEDGED) {
					connection->receivedSeqnos[i - 1] = 0;
				}
				if (seqno != 0xFE) connection->lastReceived = seqno; // Update the last received flag if the packet isn't a ping
			}
			else if (connection->receivedSeqnos[seqno - 1]) goto beginning; // The packet has already arrived
			if (seqno == 0xFE) goto beginning; // The packet was internally used as a ping
			else connection->receivedSeqnos[seqno - 1] = 1; // Mark the packet as received
		}

		printf("Sequence number is %d/", seqno);
		for (int i = 7; i >= 0; i--) putchar((seqno & (1 << i)) ? '1' : '0');
		putchar('\n');
	}
	return result;
}