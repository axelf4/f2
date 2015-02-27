#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <entityx/entityx.h>
#include <btBulletDynamicsCommon.h>

using namespace entityx;

namespace game {

	struct RigidBody {
		btMotionState *motionState;
		btCollisionShape *shape;
		btRigidBody *body;

		RigidBody(btScalar mass, btMotionState *motionState, btCollisionShape *shape, const btVector3 &inertia);
		~RigidBody();

		operator btRigidBody *() const;
	};

	class CollisionSystem : public System<CollisionSystem>, public Receiver < CollisionSystem > {
		btBroadphaseInterface *broadphase;
		btDefaultCollisionConfiguration *collisionConfiguration;
		btCollisionDispatcher *dispatcher;
		btSequentialImpulseConstraintSolver *solver;

		btIDebugDraw *debugDraw;
	public:
		btDynamicsWorld *world;

		CollisionSystem();
		~CollisionSystem();

		void configure(entityx::EventManager &event_manager);

		void update(entityx::EntityManager &entities, entityx::EventManager &events, TimeDelta dt);

		void receive(const ComponentAddedEvent<game::RigidBody> &event);
		void receive(const ComponentRemovedEvent<game::RigidBody> &event);
	};

}

#endif