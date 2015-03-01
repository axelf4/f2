#include "server.h"
#include <iostream>
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#include "netmessages.pb.h"
#include "shared.h"

using namespace std;

void accept(struct peer *peer, struct conn *connection) {
	cout << "Accepted new connection!" << endl;
}

game::Server::Server() {
	struct sockaddr_in address = net_address(0, 30000);
	server = net_peer_create(&address, 16);
	server->accept = accept;
}

game::Server::~Server() {
	net_peer_dispose(server);
}

void game::Server::update() {
	unsigned char buf[1024];
	int len;
	struct sockaddr_in from;

	net_update(server);

	while ((len = net_receive(server, buf, sizeof buf, &from)) > 0) {
		// cout << "Received: " << len << " bytes or '" << (char *)buf << "'." << endl;

		game::PacketBase packet;
		packet.ParseFromArray(buf, len - NET_SEQNO_SIZE);
		game::PacketBase_Type type = packet.type();
		if (type == game::PacketBase_Type_ClientLoginPacket) {
			cout << "ClientLoginPacket" << endl;
			game::ClientLoginPacket clientLogin = packet.clientloginpacket();
			string test = clientLogin.test();
			cout << "test: " << test << endl;
		}
		else if (type == game::PacketBase_Type_USERCMD) {
			//cout << "Move" << endl;
			game::usercmd usercmd = packet.usercmd();
			bool w = usercmd.w();
			// cout << "w: " << w << endl;
		}
	}

	for (int i = 0; i < server->numConnections; i++) {
		struct conn *client = server->connections[i];

		game::PacketBase packet;
		packet.set_type(game::PacketBase_Type_GAMEST);
		game::gamest *gamest = new game::gamest;
		gamest->set_seqno(0);
		game::entity_state *entity = gamest->add_entity();
		entity->set_id(0);

		game::vec3 *origin = new game::vec3;
		origin->set_x(0);
		origin->set_y(0);
		origin->set_z(0);
		entity->set_allocated_origin(origin);

		packet.set_allocated_gamest(gamest);
		sendPacket(server, client->addr, packet, game::PacketBase_Type_GAMEST, NET_UNRELIABLE);
	}
}