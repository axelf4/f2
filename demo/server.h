#ifndef SERVER_H
#define SERVER_H

#include <net.h>
#include <thread>
#include <entityx/entityx.h>
#include "shared.h"
#include <vector>

namespace game {

	struct ClientConnectionInfo {
		sockaddr_in address;
		unsigned int lastReceiveSeqno;
		// game::usercmd commands[256];

		ClientConnectionInfo(sockaddr_in address);
	};

	class Server {
		struct peer *server;
		entityx::EntityX entityx;
		std::shared_ptr<game::CollisionSystem> collisionSystem;
		struct model *groundMesh;
	public:
		Server();
		~Server();

		void update(float dt);
	};
}

#endif