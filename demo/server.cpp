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
	struct sockaddr_in address;
	NET_IP4_ADDR(0, 30000, &address);
	server = net_peer_create(&address, 16);
	server->accept = accept;

	collisionSystem = entityx.systems.add<CollisionSystem>();
	entityx.systems.configure();
	/* Ground body */
	struct model *groundMesh = loadMeshUsingObjLoader(RESOURCE_DIR "cs_office/cs_office.obj", 0, true, 0);
	btBvhTriangleMeshShape *groundShape = new btBvhTriangleMeshShape(groundMesh->tiva, true);
	entityx::Entity ground = game::createGround(entityx, groundShape);
	player = game::createPlayer(entityx);
}

game::Server::~Server() {
	net_peer_dispose(server);
}

void game::Server::update(float dt) {
	unsigned char buf[1024];
	int len;
	struct sockaddr_in from;

	net_update(server);

	btRigidBody *ballBody = player.component<RigidBody>()->body;
	float pitch, yaw;

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
			game::usercmd usercmd = packet.usercmd();
			game::vec3 viewangles = usercmd.viewangles();
			pitch = viewangles.x();
			yaw = viewangles.y();
			if (usercmd.w()) walk(0, yaw, MOVEMENT_SPEED, 180, ballBody);
			if (usercmd.a()) walk(0, yaw, MOVEMENT_SPEED, 270, ballBody);
			if (usercmd.s()) walk(0, yaw, MOVEMENT_SPEED, 0, ballBody);
			if (usercmd.d()) walk(0, yaw, MOVEMENT_SPEED, 90, ballBody);
			if (usercmd.space()) jump(player.component<RigidBody>().get(), collisionSystem->world);
		}
	}

	for (unsigned int i = 0; i < server->numConnections; i++) {
		struct conn *client = server->connections[i];

		game::PacketBase packet;
		packet.set_type(game::PacketBase_Type_GAMEST);
		game::gamest *gamest = packet.mutable_gamest();
		gamest->set_seqno(0);
		game::entity_state *entity = gamest->add_entity();
		entity->set_id(0);

		game::vec3 *origin = entity->mutable_origin();
		btVector3 vec = ballBody->getWorldTransform().getOrigin();
		origin->set_x(vec.x());
		origin->set_y(vec.y());
		origin->set_z(vec.z());

		sendPacket(server, client->address, packet, game::PacketBase_Type_GAMEST, NET_UNRELIABLE);
	}

	entityx.systems.update_all(dt);
}