#ifndef SHARED_H
#define SHARED_H

#include <entityx/entityx.h>
#include <btBulletDynamicsCommon.h>
#include <vmath.h>
#include <glh.h>

#include <objloader.h>
/* assimp include files. These three are usually needed. */
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace entityx;

// Converts degrees to radians.
#define degreesToRadians(angleDegrees) (angleDegrees * PI / 180.0)
// Converts radians to degrees.
#define radiansToDegrees(angleRadians) (angleRadians * 180.0 / PI)

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

	class ClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback {
	public:
		ClosestNotMeRayResultCallback(const btVector3& rayFromWorld, const btVector3& rayToWorld, btRigidBody* me) : btCollisionWorld::ClosestRayResultCallback(rayFromWorld, rayToWorld), m_me(me) {}
		virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) { return rayResult.m_collisionObject == m_me ? btScalar(1.0) : ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace); }
	protected:
		btRigidBody* m_me;
	};

	extern void walk(VEC *pos, float yaw, float distance, float direction, btRigidBody *body);

	extern void jump(game::RigidBody *ball, btDynamicsWorld *world);
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
};

extern void destroy_model(struct model *model);

extern model * loadMeshUsingObjLoader(const char *filename, GLuint program, GLuint(*load_texture)(const char *));

extern model * loadMeshUsingAssimp(const char *filename, GLuint program, bool setShape, btBvhTriangleMeshShape *&shape, GLuint(*load_texture)(const char *));

#endif