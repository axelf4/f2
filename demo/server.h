#ifndef SERVER_H
#define SERVER_H

#include <net.h>
#include <thread>
#include <entityx/entityx.h>
#include "shared.h"

#define MOVEMENT_SPEED (0.2f * 50)

namespace game {

	class Server {
		struct peer *server;
		entityx::EntityX entityx;
		std::shared_ptr<game::CollisionSystem> collisionSystem;
		entityx::Entity player;
		struct model *groundMesh;
	public:
		Server();
		~Server();

		void update(float dt);
	};
}

#endif