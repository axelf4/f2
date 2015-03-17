#ifndef SHARED_H
#define SHARED_H

#include <entityx/entityx.h>
#include <btBulletDynamicsCommon.h>
#include <vmath.h>
#include <glh.h>
#include <net.h>
#include "netmessages.pb.h"

#include <objloader.h>

using namespace entityx;

// Converts degrees to radians.
#define degreesToRadians(angleDegrees) (angleDegrees * PI / 180.0)
// Converts radians to degrees.
#define radiansToDegrees(angleRadians) (angleRadians * 180.0 / PI)

#define MOVEMENT_SPEED (0.5f * 50)

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
	public:
		btDynamicsWorld *world;
		btIDebugDraw *debugDraw;

		CollisionSystem();
		~CollisionSystem();

		void configure(entityx::EventManager &event_manager);

		void update(entityx::EntityManager &entities, entityx::EventManager &events, TimeDelta dt);

		void receive(const ComponentAddedEvent<game::RigidBody> &event);
		void receive(const ComponentRemovedEvent<game::RigidBody> &event);
	};

	class ClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback {
	public:
		ClosestNotMeRayResultCallback(const btVector3& rayFromWorld, const btVector3& rayToWorld, btRigidBody* me) : btCollisionWorld::ClosestRayResultCallback(rayFromWorld, rayToWorld), m_me(me) {}
		virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) { return rayResult.m_collisionObject == m_me ? btScalar(1.0) : ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace); }
	protected:
		btRigidBody* m_me;
	};

	extern void walk(VEC *pos, float yaw, float direction, btRigidBody *body);

	extern void jump(game::RigidBody *ball, btDynamicsWorld *world);

	extern Entity createGround(EntityX &entityx, btBvhTriangleMeshShape *groundShape);

	extern Entity createPlayer(EntityX &entityx);

	extern void sendPacket(peer *client, sockaddr_in address, game::PacketBase packet, game::PacketBase_Type type, int flag);

	extern void applyUsercmd(game::usercmd cmd, game::RigidBody *body, btDynamicsWorld *world, VEC *position);
}

struct model_node {
	// material material;
	GLuint texture;
	// f1::mesh *mesh;
	struct mesh *mesh;
	struct attrib *attributes;
	GLsizei stride;
	GLsizei vertexCount;
	GLsizei indexCount;

	float *vertices;
	unsigned int *indices;
};

struct model {
	model_node **nodes;
	size_t nodeCount;
	btTriangleIndexVertexArray *tiva;
	obj_model *obj_model;
};

extern void destroy_model(struct model *model);

extern model * loadMeshUsingObjLoader(const char *filename, GLuint program, bool setShape, GLuint(*load_texture)(const char *));

#endif