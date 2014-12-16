#include "net.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

struct peer * net_peer_create(struct addr *address, unsigned short maxConnections) {
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

	if (address != 0) {
		struct sockaddr_in RecvAddr;
		RecvAddr.sin_family = AF_INET;
		RecvAddr.sin_port = htons(6622); // TODO the address field for is not an ushort
		// RecvAddr.sin_port = htons(address->port);
		// RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		RecvAddr.sin_addr.s_addr = INADDR_ANY;

		iResult = bind(Socket, (struct sockaddr *) &RecvAddr, sizeof(RecvAddr));
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

	for (int i = 0; i < peer->numConnections; i++) {
		free(peer->connections[i]);
	}
	free(peer->connections);

	WSACleanup();

	free(peer);
}

void net_update(struct peer *peer) {
	// TODO first write this berkeley code then optimize for windows with WSA*.
	char buf[1024];
	struct sockaddr_in from;
	int fromlen = sizeof(from), result;
	if ((result = recvfrom(peer->socket, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen)) == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK) printf("recvfrom failed with error %d\n", result);
	}
	else if (result > 0) {
		int newConnection = 1;
		for (int i = 0; i < peer->numConnections; i++) {
			struct sockaddr_in connAddr = peer->connections[i]->addr;
			// if (memcmp(&from, &connAddr, sizeof(struct sockaddr_in)) == 0) {
			if (SOCK_ADDR_EQ_ADDR(&from, &connAddr) && SOCK_ADDR_EQ_PORT(&from, &connAddr)) {
				newConnection = 0;
			}
		}
		if (newConnection) {
			struct conn *conn = malloc(sizeof(struct conn));
			conn->addr = from;
			peer->connections[peer->numConnections++] = conn;

			printf("New connection.\n");
		}
		else printf("Existing connection.\n");

		printf("Is first char in received message set? %c\n", *buf);
		printf("Is first char in received message set? %u\n", *buf << 7);
		printf("Received: %d bytes or %s\n", result, buf + 1);

		sendto(peer->socket, buf, result, 0, (struct sockaddr *)&from, fromlen);
	}

	/*struct conn *conn = malloc(sizeof(struct conn));
	conn->socket = ClientSocket;
	peer->connections[peer->numConnections++] = conn;*/
}

void net_send(struct peer *peer, const char *buf, int len) {
	printf("sending bytes\n");
	struct sockaddr_in to = { .sin_family = AF_INET, .sin_port = htons(DEFAULT_PORT), .sin_addr.s_addr = inet_addr("127.0.0.1") };
	int tolen = sizeof(to);

	int result = sendto(peer->socket, buf - 1, len + 1, 0, (struct sockaddr *)&to, tolen);
	if (result == SOCKET_ERROR) {
		printf("sendto failed with error: %d\n", WSAGetLastError());
		closesocket(peer->socket);
		WSACleanup();
		return;
	}
}

char * net_packet(int len) {
	char *buf = (char *)malloc(len + 1);
	*buf = 0x80;
	return buf + 1;
}