#include "shared.h"
#include "GLDebugDrawer.h"
#include <string>

using namespace std;
using namespace game;

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
	// Bullet Physics initialization
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

void game::walk(VEC *pos, float yaw, float direction, btRigidBody *body) {
	float xaccel = MOVEMENT_SPEED * (float)sin(degreesToRadians(yaw + direction)), zaccel = MOVEMENT_SPEED * (float)cos(degreesToRadians(yaw + direction));
	if (pos != 0) {
		VEC accel = VectorSet(xaccel, 0, zaccel, 0);
		*pos = VectorAdd(*pos, accel);
	}

	btVector3 velocity = body->getLinearVelocity();
	body->setLinearVelocity(btVector3(velocity.x() + xaccel * 5, velocity.y(), velocity.z() + zaccel * 5));
	// body->applyCentralImpulse(btVector3(xaccel * 10, 0, zaccel * 10));
}

void game::jump(game::RigidBody *ball, btDynamicsWorld *world) {
	// Jumping
	/*btTransform ballTransform;
	ball->motionState->getWorldTransform(ballTransform);
	btVector3 pos = ballTransform.getOrigin(), btFrom(pos.x(), pos.y(), pos.z()), btTo(pos.x(), pos.y() - 1, pos.z());
	ClosestNotMeRayResultCallback res(btFrom, btTo, ball->body);
	world->rayTest(btFrom, btTo, res);
	if (res.hasHit()){
		ball->body->applyCentralImpulse(btVector3(0.0f, 2.5f * 100, 0.f));
		cout << "jump" << endl;
	}*/

	btTransform transform;
	ball->motionState->getWorldTransform(transform);
	// btVector3 down = -xform.getBasis()[1];
	btVector3 down = transform.getOrigin();
	// cout << down.getX() << down.getY() << down.getZ() << endl;
	// down.normalize();
	btScalar halfHeight = btScalar(5) / 2;
	btVector3 from(transform.getOrigin()), to(from + down * halfHeight * btScalar(1.1));
	ClosestNotMeRayResultCallback rayCallback(from, to, ball->body);
	rayCallback.m_closestHitFraction = 1.0;
	world->rayTest(from, to, rayCallback);
	if (rayCallback.hasHit())
	{
		if (rayCallback.m_closestHitFraction < btScalar(0.01)) {
			cout << "jump" << endl;
			// btTransform xform;
			// ball->motionState->getWorldTransform(xform);
			// btVector3 up = xform.getBasis()[1];
			// up.normalize();
			//btScalar magnitude = (btScalar(1.0) / ball->body->getInvMass()) * btScalar(8.0);
			// ball->body->applyCentralImpulse(up * magnitude);
			// btVector3 origin = transform.getOrigin();
			// transform.setOrigin(btVector3(origin.x(), origin.y() + 1, origin.z()));
			ball->body->applyCentralImpulse(btVector3(0, 2.5f * 100, 0));
		}
	}
}

Entity game::createGround(EntityX &entityx, btBvhTriangleMeshShape *groundShape) {
	// btCollisionShape *groundShape = /* new btStaticPlaneShape(btVector3(0, 1, 0), 1) */ new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
	btDefaultMotionState *groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
	entityx::Entity ground = entityx.entities.create();
	ground.assign<RigidBody>(0, groundMotionState, groundShape, btVector3(0, 0, 0));
	ground.component<RigidBody>()->body->setFriction(1.0f); // 5.2

	return ground;
}

Entity game::createPlayer(EntityX &entityx) {
	/* Ball body */
	// btCollisionShape *ballShape = /* new btSphereShape(1) */ new btBoxShape(btVector3(5, 5, 5));
	btCollisionShape *ballShape = new btCylinderShape(btVector3(5, 5, 5));
	btDefaultMotionState* ballMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 50, 0)));
	btScalar mass = 1;
	btVector3 ballInertia(0, 0, 0);
	ballShape->calculateLocalInertia(mass, ballInertia);
	entityx::Entity ball = entityx.entities.create();
	ball.assign<RigidBody>(mass, ballMotionState, ballShape, ballInertia);
	btRigidBody *ballBody = ball.component<RigidBody>()->body;
	ballBody->setSleepingThresholds(btScalar(.0f), btScalar(.0f));
	ballBody->setAngularFactor(.0f);
	ballBody->setRestitution(0.f);
	ballBody->setCcdMotionThreshold(1.0f);
	ballBody->setCcdSweptSphereRadius(0.2f);

	return ball;
}

void game::sendPacket(peer *client, sockaddr_in address, game::PacketBase packet, game::PacketBase_Type type, int flag) {
	packet.set_type(type);

	int len = packet.ByteSize() + NET_SEQNO_SIZE;
	unsigned char *buffer = new unsigned char[len];
	packet.SerializeToArray(buffer, len - NET_SEQNO_SIZE);
	net_send(client, buffer, len, address, flag);
}

void game::applyUsercmd(game::usercmd cmd, game::RigidBody *body, btDynamicsWorld *world, VEC *position) {
	game::vec3 viewangles = cmd.viewangles();
	float pitch = viewangles.x(), yaw = viewangles.y();

	if (cmd.w()) walk(position, yaw, 180, body->body);
	if (cmd.a()) walk(position, yaw, 270, body->body);
	if (cmd.s()) walk(position, yaw, 0, body->body);
	if (cmd.d()) walk(position, yaw, 90, body->body);
	if (!(cmd.w() || cmd.a() || cmd.s() || cmd.d())) {
		body->body->setLinearVelocity(btVector3(0, body->body->getLinearVelocity().y(), 0));
	}
	if (cmd.space()) {
		if (position != 0) *position = VectorAdd(*position, VectorSet(0, MOVEMENT_SPEED, 0, 0)); // position.y += MOVEMENT_SPEED;

		jump(body, world);
	}
}

void destroy_model(struct model *model) {
	for (size_t i = 0; i < model->nodeCount; i++) {
		model_node *node = model->nodes[i];
		if (node->texture != 0) glDeleteTextures(1, &node->texture);
		if (node->mesh != 0) destroy_mesh(node->mesh);
		if (node->attributes != 0) free(node->attributes);

		// if (node->vertices != 0) delete[] node->vertices;
		// if (node->indices != 0) delete[] node->indices;

		delete node;
	}
	delete[] model->nodes;
	destroy_obj_model(model->obj_model);
	if (model->tiva != 0) model->tiva;
	delete model;
}

model * loadMeshUsingObjLoader(const char *filename, GLuint program, bool setShape, GLuint(*load_texture)(const char *)) {
	struct obj_model *scene = load_obj_model(filename);
	if (!scene) {
		std::cerr << "Error loading model: " << filename << std::endl;
		return 0;
	}
	model *model = new struct model;
	model->nodes = new model_node*[scene->numParts];
	model->nodeCount = scene->numParts;

	btTriangleIndexVertexArray *tiva;
	if (setShape) {
		tiva = new btTriangleIndexVertexArray();
	}

	GLuint *textures = new GLuint[scene->numMaterials];
	for (unsigned int i = 0; i < scene->numMaterials; i++) {
		struct mtl_material *material = scene->materials[i];
		if (material->texPath != 0 && load_texture != 0) {
			// Material has texture
			string fullpath(filename);
			fullpath = fullpath.substr(0, fullpath.rfind('/') + 1).append(material->texPath);
			textures[i] = load_texture(fullpath.c_str());
		}
		else textures[i] = 0;
	}

	for (unsigned int i = 0; i < scene->numParts; i++) {
		struct obj_model_part *part = scene->parts[i];
		model_node *node = model->nodes[i] = new model_node;
		unsigned int vertexCount = node->vertexCount = part->vertexCount, indexCount = node->indexCount = part->indexCount;
		float *vertices = node->vertices = part->vertices;
		unsigned int *indices = node->indices = part->indices;
		node->texture = textures[part->materialIndex];

		size_t k = 0;
		node->attributes = (struct attrib *) malloc(sizeof(struct attrib) * (2 + scene->hasUVs + scene->hasNorms));
		node->attributes[k++] = { program ? glGetAttribLocation(program, "vertex") : 0, 3, 0 };
		if (scene->hasUVs) node->attributes[k++] = { program ? glGetAttribLocation(program, "texCoord") : 0, 2, 0 };
		if (scene->hasNorms) node->attributes[k++] = { program ? glGetAttribLocation(program, "normal") : 0, 3, 0 };
		node->attributes[k++] = NULL_ATTRIB;
		node->stride = calculate_stride(node->attributes);

		if (program != 0) {
			node->mesh = create_mesh(1);
			glBindBuffer(GL_ARRAY_BUFFER, node->mesh->vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexCount, vertices, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, node->mesh->ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indexCount, indices, GL_STATIC_DRAW);
		}

		if (setShape) {
			btIndexedMesh indexedMesh;
			indexedMesh.m_vertexType = PHY_FLOAT;
			indexedMesh.m_numVertices = vertexCount;
			indexedMesh.m_vertexBase = (const unsigned char*)vertices;
			indexedMesh.m_vertexStride = node->stride;
			indexedMesh.m_indexType = PHY_INTEGER;
			indexedMesh.m_numTriangles = indexCount / 3;
			indexedMesh.m_triangleIndexBase = (const unsigned char*)indices;
			indexedMesh.m_triangleIndexStride = 3 * sizeof(unsigned int);
			tiva->addIndexedMesh(indexedMesh);
		}
	}

	if (setShape) model->tiva = tiva;
	model->obj_model = scene;

	delete[] textures;
	return model;
}