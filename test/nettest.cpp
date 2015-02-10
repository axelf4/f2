#include <iostream>
#include <net.h>
#include <string.h>
#include <thread>
#include <mutex>
#include <Windows.h>

using namespace std;

#define DEFAULT_PORT 6622
#define MAX_CLIENTS 10

struct conn *clientConn;
int receivedNum = 0;

void accept(struct peer *peer, struct conn *connection) {
	cout << "Accepted new connection!" << endl;
	clientConn = connection;
}

void readPeer(struct peer *peer, std::mutex *mtx, bool *continuing, bool client) {
	while (*continuing) {
		{
			std::unique_lock<std::mutex> lck(*mtx);
			net_update(peer);

			unsigned char buf[1024];
			int len;
			struct sockaddr_in from;
			while ((len = net_receive(peer, buf, sizeof buf, &from)) > 0) {
				printf("Received: %d bytes or '%s'. Nr %d.\n", len, buf, ++receivedNum);

				//char *sendBuf = new char[len];
				//memcpy(sendBuf, buf, len);
				// if (!client) net_send(peer, sendBuf, len, from, 0);
			}

			// if (len == -1) break;
		}

		Sleep(10);
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
		cout << (client ? "Client" : "Server") << ": 0=stop, 1=send; ";
		int option;
		cin >> option;
		std::unique_lock<std::mutex> lck(mtx);
		if (option == 0) {
			continuing = false;
		}
		else {
			for (int i = 0; i < option; i++) { // 252
				char *string = "HelloWorld!";
				int buflen = strlen(string) + 1 + NET_SEQNO_SIZE; // One for the null-terminator and one for internal usage
				unsigned char *buf = (unsigned char *)malloc(sizeof(char) * buflen);
				strcpy((char *)buf, string);

				net_send(peer, buf, buflen, client ? net_address("127.0.0.1", DEFAULT_PORT) : clientConn->addr, 1);
			}
		}
	}

	readThread.join();

	net_peer_dispose(peer);

	return 0;
}