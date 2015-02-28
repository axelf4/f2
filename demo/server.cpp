#include "server.h"
#include <iostream>
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include "netmessages.pb.h"

using namespace std;

void accept(struct peer *peer, struct conn *connection) {
	cout << "Accepted new connection!" << endl;
}

game::Server::Server() {
	struct sockaddr_in address = net_address(0, 30000);
	server = net_peer_create(&address, 16);
	server->accept = accept;

	continuing = false;
}

game::Server::~Server() {
	if (continuing) {
		continuing = false;
		inputThread.join();
	}
	net_peer_dispose(server);
}

void game::Server::readPeer() {
	unsigned char buf[1024];
	int len;
	struct sockaddr_in from;

	net_update(server);

	while ((len = net_receive(server, buf, sizeof buf, &from)) > 0) {
		cout << "Received: " << len << " bytes or '" << (char *)buf << "'." << endl;

		game::PacketBase packet;
		packet.ParseFromArray(buf, len - NET_SEQNO_SIZE);
		game::PacketBase_Type type = packet.type();
		if (type == game::PacketBase_Type_ClientLoginPacket) {
			cout << "ClientLoginPacket" << endl;
			game::ClientLoginPacket clientLogin = packet.clientloginpacket();
			string test = clientLogin.test();
			cout << "test: " << test << endl;
		}
		else if (type == game::PacketBase_Type_Move) {
			cout << "Move" << endl;
			game::Msg_Move move = packet.move();
			bool w = move.w();
			cout << "w: " << w << endl;
		}
	}
}

/*void shutdownServerInstance(DWORD fdwCtrlType) {
	serverInstance->continuing = false;
	}*/

void game::Server::loopRead() {
	while (continuing) {
		readPeer();
		Sleep(300);
	}
}

void game::Server::startServer() {
	// SetConsoleCtrlHandler((PHANDLER_ROUTINE)shutdownServerInstance, TRUE);

	continuing = true;

	inputThread = std::thread(&Server::loopRead, this);
}