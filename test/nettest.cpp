#include <iostream>
#include <net.h>
#include <string.h>
#include <thread>
#include <mutex>
#include <Windows.h>

using namespace std;

#define MAX_CLIENTS 10

struct conn *clientConn;

void accept(struct peer *peer, struct conn *connection) {
	cout << "Accepted new connection!" << endl;
	clientConn = connection;
}

void readPeer(struct peer *peer, std::mutex *mtx, bool *continuing, bool client) {
	while (*continuing) {
		{
			std::unique_lock<std::mutex> lck(*mtx);
			net_update(peer);

			char buf[1024];
			int len;
			struct sockaddr_in from;
			while ((len = net_receive(peer, buf, sizeof buf, &from)) > 0) {
				printf("Received: %d bytes or '%s'.\n", len, buf);

				//char *sendBuf = new char[len];
				//memcpy(sendBuf, buf, len);
				// if (!client) net_send(peer, sendBuf, len, from, 0);
			}

			// if (len == -1) break;
		}

		// Sleep(500);
	}
}

int main() {
	cout << "Client? ";
	bool client;
	cin >> client;
	cout << (client ? "Client" : "Server") << '!' << endl;

	net_initialize();

	struct peer *peer;
	if (!client) {
		struct sockaddr_in address = net_address(0, DEFAULT_PORT);
		peer = net_peer_create(&address, MAX_CLIENTS);
	}
	else {
		peer = net_peer_create(0, 1);
	}
	peer->accept = accept;

	/*int len = 64;
	char *buf = net_packet(len);
	strncpy(buf, "HelloWorld!", len);*/

	std::mutex mtx;
	bool continuing = true;
	std::thread readThread;
	readThread = std::thread(readPeer, peer, &mtx, &continuing, client);

	cout << "Scanning for input..." << endl;
	while (continuing) {
		cout << "0:stop, 1:send; ";
		int option;
		cin >> option;
		std::unique_lock<std::mutex> lck(mtx);
		if (option == 0) {
			continuing = false;
		}
		else {
			char *string = "HelloWorld!";
			int buflen = strlen(string) + 2; // One for the null-terminator and one for internal usage
			char *buf = new char[buflen];
			strcpy(buf, string);

			net_send(peer, buf, buflen, client ? net_address("127.0.0.1", DEFAULT_PORT) : clientConn->addr, 1);
		}
	}

	readThread.join();

	net_peer_dispose(peer);

	// system("pause");

	return 0;
}