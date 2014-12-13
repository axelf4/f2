#include <iostream>
#include "net.h"
#include <Windows.h>
#include <thread>

using namespace std;

int main() {
	cout << "Client? ";
	bool client;
	cin >> client;

	net_initialize();

	if (!client) {
		cout << "Server!" << endl;
		// std::thread t1(server_task);
		net_server_socket2();
	} else {
		cout << "Client!" << endl;
		net_client_socket2("localhost");
	}

	system("pause");

	return 0;
}