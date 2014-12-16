#include <iostream>
#include "net.h"
#include <Windows.h>
#include <thread>
#include <string.h>

using namespace std;

#define MAX_CLIENTS 10

int main() {
	cout << "Client? ";
	bool client;
	cin >> client;

	net_initialize();

	struct peer *peer;
	if (!client) {
		cout << "Server!" << endl;

		struct addr addr = { 0, DEFAULT_PORT };
		peer = net_peer_create(&addr, 1);
	}
	else {
		peer = net_peer_create(0, MAX_CLIENTS);
	}

	int len = 64;
	char *buf = net_packet(len);
	strncpy(buf, "HelloWorld!", len);

	cout << "Scanning for input..." << endl;
	bool continuing = true;
	while (continuing) {
		cout << "0:stop, 1:receive, 2:send; ";
		int option;
		cin >> option;
		switch (option) {
		case 0:
			continuing = false;
			break;
		case 2:
			net_send(peer, buf, 64);
		case 1:
			net_update(peer);
			break;
		}
	}
	net_peer_dispose(peer);

	system("pause");

	return 0;
}