#include "server.h"
#include <iostream>
#define WIN32_LEAN_AND_MEAN 1
#include "netmessages.pb.h"

using namespace std;

game::ClientConnectionInfo::ClientConnectionInfo(sockaddr_in address) : address(address), lastReceiveSeqno(0) {}

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
}

game::Server::~Server() {
	net_peer_dispose(server);
}

void game::Server::update(float dt) {
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
			Entity player = game::createPlayer(entityx);
			player.assign<ClientConnectionInfo>(from);
		}
		else if (type == game::PacketBase_Type_USERCMD) {
			ComponentHandle<ClientConnectionInfo> clientInfo;
			bool foundPlayer = false;
			Entity player;
			for (Entity entity : entityx.entities.entities_with_components(clientInfo)) {
				if (SOCK_ADDR_EQ_ADDR(&from, &clientInfo->address) && SOCK_ADDR_EQ_PORT(&from, &clientInfo->address)) {
					player = entity;
					foundPlayer = true;
					break;
				}
			}
			if (!foundPlayer) {
				return;
			}
			btRigidBody *ballBody = player.component<RigidBody>()->body;

			game::usercmd usercmd = packet.usercmd();
			if (usercmd.seqno() < clientInfo->lastReceiveSeqno) continue;
			else clientInfo->lastReceiveSeqno = usercmd.seqno();
			/*game::vec3 viewangles = usercmd.viewangles();
			pitch = viewangles.x();
			yaw = viewangles.y();
			if (usercmd.w()) walk(0, yaw, MOVEMENT_SPEED, 180, ballBody);
			if (usercmd.a()) walk(0, yaw, MOVEMENT_SPEED, 270, ballBody);
			if (usercmd.s()) walk(0, yaw, MOVEMENT_SPEED, 0, ballBody);
			if (usercmd.d()) walk(0, yaw, MOVEMENT_SPEED, 90, ballBody);
			if (usercmd.space()) jump(player.component<RigidBody>().get(), collisionSystem->world);*/
			applyUsercmd(usercmd, player.component<RigidBody>().get(), collisionSystem->world, 0);
		}
	}

	/*for (unsigned int i = 0; i < server->numConnections; i++) {
		struct conn *client = server->connections[i];*/
	ComponentHandle<ClientConnectionInfo> clientInfo;
	for (Entity player : entityx.entities.entities_with_components(clientInfo)) {
		btRigidBody *ballBody = player.component<RigidBody>()->body;

		game::PacketBase packet;
		packet.set_type(game::PacketBase_Type_GAMEST);
		game::gamest *gamest = packet.mutable_gamest();
		gamest->set_seqno(clientInfo->lastReceiveSeqno);
		game::entity_state *entity = gamest->add_entity();
		entity->set_id(0);

		btVector3 vec;

		game::vec3 *origin = entity->mutable_origin();
		vec = ballBody->getWorldTransform().getOrigin();
		origin->set_x(vec.x());
		origin->set_y(vec.y());
		origin->set_z(vec.z());

		game::vec3 *velocity = entity->mutable_velocity();
		vec = ballBody->getLinearVelocity();
		velocity->set_x(vec.x());
		velocity->set_y(vec.y());
		velocity->set_z(vec.z());

		sendPacket(server, clientInfo->address, packet, game::PacketBase_Type_GAMEST, NET_UNRELIABLE);
	}

	entityx.systems.update_all(dt);
}
