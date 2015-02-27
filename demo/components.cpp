#include "components.h"
#include "GLDebugDrawer.h"

game::RigidBody::RigidBody(btScalar mass, btMotionState *motionState, btCollisionShape *shape, const btVector3 &inertia) : motionState(motionState), shape(shape) {
	btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, shape, inertia);
	body = new btRigidBody(rigidBodyCI);
}

game::RigidBody::~RigidBody() {
	delete motionState;
	delete shape;
	delete body;
}

game::RigidBody::operator btRigidBody *() const { return body; }

void myTickCallback(btDynamicsWorld *world, btScalar timeStep) {
	for (int i = 0; i < world->getNumCollisionObjects(); i++) {
		btCollisionObject *colObj = world->getCollisionObjectArray()[i];
		btRigidBody *body = btRigidBody::upcast(colObj);
		btVector3 velocity = body->getLinearVelocity(),
			velocity2(velocity);
		velocity2.setY(0);
		btScalar speed = velocity2.length();
		const float maxSpeed = 150;
		if (speed > 350) {
			velocity2 *= 350 / speed;
			btVector3 velocity(velocity2.x(), velocity.y(), velocity2.z());
			body->setLinearVelocity(velocity);
		}
	}

}

game::CollisionSystem::CollisionSystem() {
	collisionConfiguration = new btDefaultCollisionConfiguration;
	broadphase = new btDbvtBroadphase;
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	solver = new btSequentialImpulseConstraintSolver;
	world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
	world->setGravity(btVector3(0, -9.81f * 70, 0));

	debugDraw = new GLDebugDrawer();
	// debugDraw->setDebugMode(btIDebugDraw::DBG_DrawAabb | btIDebugDraw::DBG_DrawWireframe);
	world->setDebugDrawer(debugDraw);

	world->setInternalTickCallback(&myTickCallback);
}

game::CollisionSystem::~CollisionSystem() {
	delete debugDraw;

	/*for (int i = 0; i < world->getNumCollisionObjects(); i++) {
		btCollisionObject* obj = world->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState()) {
		delete body->getMotionState();
		}
		world->removeCollisionObject(obj);
		delete obj;
		}*/

	delete world;
	delete solver;
	delete broadphase;
	delete dispatcher;
	delete collisionConfiguration;
}

void game::CollisionSystem::configure(entityx::EventManager &event_manager) {
	event_manager.subscribe<entityx::ComponentAddedEvent<game::RigidBody>>(*this);
	event_manager.subscribe<entityx::ComponentRemovedEvent<game::RigidBody>>(*this);
}

void game::CollisionSystem::update(entityx::EntityManager &entities, entityx::EventManager &events, TimeDelta dt) {
	world->stepSimulation(dt, 100); // 1 / 60.f
}

void game::CollisionSystem::receive(const ComponentAddedEvent<game::RigidBody> &event) {
	world->addRigidBody(event.component->body);
}

void game::CollisionSystem::receive(const ComponentRemovedEvent<game::RigidBody> &event) {
	world->removeRigidBody(event.component->body);
}