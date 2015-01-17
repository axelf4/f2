#include <iostream>

#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>

#include <glh.h>
#include <vmath.h>
#include <objloader.h>

#include <SOIL.h>
// #include "obj_loader.h"
/* assimp include files. These three are usually needed. */
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <btBulletDynamicsCommon.h>

#include "shaders.h"
#include "GLDebugDrawer.h"

// #define WINDOW_TITLE "Point and Click Adventures"
#define WINDOW_TITLE "Call of Duty: Avancerad V\xC3\xA4lf\xC3\xA4rd"
// 640/480, 800/600, 1366/768
#define WINDOW_WIDTH 800.f
#define WINDOW_HEIGHT 600.f
#define FPS 60
#define MOUSE_SENSITIVITY 0.006f
#define MOVEMENT_SPEED (0.2f * 50)

#ifndef RESOURCE_DIR
#define RESOURCE_DIR "resource/"
#endif

// Converts degrees to radians.
#define degreesToRadians(angleDegrees) (angleDegrees * M_PI / 180.0)
// Converts radians to degrees.
#define radiansToDegrees(angleRadians) (angleRadians * 180.0 / M_PI)

using namespace std;

void update_camera(float *yaw, float yaw_amount, float *pitch, float pitch_amount) {
	*yaw -= yaw_amount;
	*pitch -= pitch_amount;
}

void walk(VEC *pos, float yaw, float distance, float direction, btRigidBody *body) {
	float xaccel = distance * (float)sin(degreesToRadians(yaw + direction)), zaccel = distance * (float)cos(degreesToRadians(yaw + direction));
	VEC accel = VectorSet(xaccel, 0, zaccel, 0);
	*pos = VectorAdd(*pos, accel);

	// body->setLinearVelocity(btVector3(xaccel * 50, yaccel, zaccel * 50));
	body->applyCentralImpulse(btVector3(xaccel * 10, 0, zaccel * 10));
}

typedef struct {
	const char *name;

	float ambient[3];
	float diffuse[3];
	float specular[3];
	float transmittance[3];
	float emission[3];
	float shininess;
	float ior; // index of refraction
	float dissolve; // 1 == opaque; 0 == fully transparent
	int illum; // illumination model (see http://www.fileformat.info/format/material/)

	const char *ambient_texname;
	const char *diffuse_texname;
	const char *specular_texname;
	const char *normal_texname;
	// std::map<std::string, std::string> unknown_parameter;
} material;

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

void destroy_model(struct model *model) {
	for (size_t i = 0; i < model->nodeCount; i++) {
		model_node *node = model->nodes[i];
		glDeleteTextures(1, &node->texture);
		destroy_mesh(node->mesh);
		free(node->attributes);

		if (node->vertices != 0) delete[] node->vertices;
		if (node->indices != 0) delete[] node->indices;

		delete node;
	}
	if (model->tiva != 0) model->tiva;
	delete[] model->nodes;
	delete model;
}

// TODO rename to get_texture
GLuint load_texture(const char *filename) {
	GLuint texture = SOIL_load_OGL_texture(
		filename,
		SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
	if (texture == 0) printf("SOIL loading error: %s\n", SOIL_last_result());
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	return texture;
}

// TODO merge into load_texture
GLuint create_null_texture(int width, int height) {
	// Create an empty white texture. This texture is applied to OBJ models
	// that don't have any texture maps. This trick allows the same shader to
	// be used to draw the OBJ model with and without textures applied.

	int pitch = ((width * 32 + 31) & ~31) >> 3; // align to 4-byte boundaries
	GLubyte *pixels = new GLubyte[pitch * height] /* { 255 } */;
	for (int i = 0; i < pitch * height; i++) pixels[i] = 255;
	GLuint texture = 0;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, &pixels[0]);

	return texture;
}

model * loadMeshUsingObjLoader(const char *filename, GLuint program) {
	struct obj_model *scene = load_obj_model(filename);
	if (!scene) {
		std::cerr << "Error loading model: " << filename << std::endl;
		return 0;
	}
	model *model = new struct model;
	model->nodes = new model_node*[scene->numParts];
	model->nodeCount = scene->numParts;

	GLuint *textures = new GLuint[scene->numMaterials];
	for (unsigned int i = 0; i < scene->numMaterials; i++) {
		struct mtl_material *material = scene->materials[i];
		if (material->texPath != 0) {
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

		size_t k = 0;
		node->attributes = (struct attrib *) malloc(sizeof(struct attrib) * (2 + scene->hasUVs + scene->hasNorms));
		node->attributes[k++] = { glGetAttribLocation(program, "vertex"), 3, 0 };
		if (scene->hasUVs) node->attributes[k++] = { glGetAttribLocation(program, "texCoord"), 2, 0 };
		if (scene->hasNorms) node->attributes[k++] = { glGetAttribLocation(program, "normal"), 3, 0 };
		node->attributes[k++] = NULL_ATTRIB;
		node->mesh = create_mesh(0);
		node->stride = calculate_stride(node->attributes);

		glBindBuffer(GL_ARRAY_BUFFER, node->mesh->vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (node->vertexCount = part->vertexCount), part->vertices, GL_STATIC_DRAW);
		node->vertices = (float *)(node->indices = 0);
		node->texture = textures[part->materialIndex];
	}

	delete[] textures;
	destroy_obj_model(scene);
	return model;
}

model * loadMeshUsingAssimp(const char *filename, GLuint program, bool setShape, btBvhTriangleMeshShape *&shape) {
	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFile(filename, aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_SortByPType |
		aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes);

	if (!scene) {
		std::cerr << "Error loading model: " << filename << ", " << importer.GetErrorString() << std::endl;
		throw 0;
	}

	model *model = new struct model;
	model->nodes = new model_node*[scene->mNumMeshes];
	model->nodeCount = scene->mNumMeshes;

	cout << "setshape: " << setShape << endl;
	btTriangleIndexVertexArray *tiva;
	if (setShape) {
		tiva = new btTriangleIndexVertexArray();
	}

	GLuint *textures = new GLuint[scene->mNumMaterials];
	for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
		const struct aiMaterial *material = scene->mMaterials[i];
		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString path;
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
				string fullpath(filename);
				fullpath = fullpath.substr(0, fullpath.rfind('/') + 1).append(path.C_Str());
				textures[i] = load_texture(fullpath.c_str());
			}
		}
		else textures[i] = 0;
	}

	for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
		const struct aiMesh *mesh = scene->mMeshes[i];
		model_node *node = model->nodes[i] = new model_node;

		unsigned int vertexCount = node->vertexCount = mesh->mNumVertices * (mesh->HasPositions() * 3 + mesh->HasTextureCoords(0) * 2 + mesh->HasNormals() * 3);
		float *vertices = new float[vertexCount];
		for (unsigned int j = 0, k = 0; j < mesh->mNumVertices; j++) {
			if (mesh->HasPositions()) {
				vertices[k++] = mesh->mVertices[j].x;
				vertices[k++] = mesh->mVertices[j].y;
				vertices[k++] = mesh->mVertices[j].z;
			}
			if (mesh->HasTextureCoords(0)) {
				vertices[k++] = mesh->mTextureCoords[0][j].x;
				vertices[k++] = mesh->mTextureCoords[0][j].y;
			}
			if (mesh->HasNormals()) {
				vertices[k++] = mesh->mNormals[j].x;
				vertices[k++] = mesh->mNormals[j].y;
				vertices[k++] = mesh->mNormals[j].z;
			}
		}
		unsigned int indexCount = node->indexCount = mesh->mNumFaces * 3;
		unsigned int *indices = new unsigned int[indexCount];
		for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
			const aiFace &face = mesh->mFaces[j]; // Assume that faces're triangles; 3 indices.
			indices[j * 3 + 0] = face.mIndices[0];
			indices[j * 3 + 1] = face.mIndices[1];
			indices[j * 3 + 2] = face.mIndices[2];
		}

		size_t j = 0;
		node->attributes = (struct attrib *) malloc(sizeof(struct attrib) * (2 + mesh->HasTextureCoords(0) + mesh->HasNormals()));
		if (mesh->HasPositions()) node->attributes[j++] = { glGetAttribLocation(program, "vertex"), 3, 0 };
		if (mesh->HasTextureCoords(0)) node->attributes[j++] = { glGetAttribLocation(program, "texCoord"), 2, 0 };
		if (mesh->HasNormals()) node->attributes[j++] = { glGetAttribLocation(program, "normal"), 3, 0 };
		node->attributes[j++] = NULL_ATTRIB;
		node->mesh = create_mesh(1);
		node->stride = calculate_stride(node->attributes);

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

		glBindBuffer(GL_ARRAY_BUFFER, node->mesh->vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexCount, vertices, GL_STATIC_DRAW);
		node->vertices = vertices; // delete[] vertices;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, node->mesh->ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indexCount, indices, GL_STATIC_DRAW);
		node->indices = indices; // delete[] indices;
		node->texture = textures[mesh->mMaterialIndex];
	}

	if (setShape) {
		model->tiva = tiva;
		shape = new btBvhTriangleMeshShape(tiva, true);
	}

	delete[] textures;
	return model;
}

GLuint setupSkyboxTexture() {
	/* load 6 images into a new OpenGL cube map, forcing RGB */
	GLuint cubemap = SOIL_load_OGL_cubemap(
		RESOURCE_DIR "zpos.png",
		RESOURCE_DIR "zneg.png",
		RESOURCE_DIR "ypos.png",
		RESOURCE_DIR "yneg.png",
		RESOURCE_DIR "xpos.png",
		RESOURCE_DIR "xneg.png",
		SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return cubemap;
}

struct game_world;

struct game_entity {
	btDynamicsWorld *world;
	btMotionState *motionState;
	btCollisionShape *shape;
	btRigidBody *body;

	game_entity(btDynamicsWorld *world, btScalar mass, btMotionState *motionState, btCollisionShape *shape, const btVector3 &inertia) : world(world), motionState(motionState), shape(shape) {
		btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, shape, inertia);
		body = new btRigidBody(rigidBodyCI);
	}

	~game_entity() {
		world->removeRigidBody(body);
		delete motionState;
		delete shape;
		delete body;
	}
};

game_entity *ball;

void myTickCallback(btDynamicsWorld *world, btScalar timeStep) {
	btVector3 velocity = ball->body->getLinearVelocity(), velocity2(velocity);
	velocity2.setY(0);
	btScalar speed = velocity2.length();
	const float maxSpeed = 150;
	if (speed > 350) {
		velocity2 *= 350 / speed;
		btVector3 velocity(velocity2.x(), velocity.y(), velocity2.z());
		ball->body->setLinearVelocity(velocity);
	}
}

struct game_world {
	btBroadphaseInterface *broadphase;
	btDefaultCollisionConfiguration *collisionConfiguration;
	btCollisionDispatcher *dispatcher;
	btSequentialImpulseConstraintSolver *solver;
	btDynamicsWorld *world;

	btIDebugDraw *debugDraw;

	void init() {
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

	~game_world() {
		delete debugDraw;

		for (int i = 0; i < world->getNumCollisionObjects(); i++) {
			btCollisionObject* obj = world->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(obj);
			if (body && body->getMotionState()) {
				delete body->getMotionState();
			}
			world->removeCollisionObject(obj);
			delete obj;
		}

		delete world;
		delete solver;
		delete broadphase;
		delete dispatcher;
		delete collisionConfiguration;
	}

	void add_entity(game_entity *entity) {
		world->addRigidBody(entity->body);
	}
};

class ClosestNotMeRayResultCallback : public btCollisionWorld::ClosestRayResultCallback {
public:
	ClosestNotMeRayResultCallback(const btVector3& rayFromWorld, const btVector3& rayToWorld, btRigidBody* me) : btCollisionWorld::ClosestRayResultCallback(rayFromWorld, rayToWorld), m_me(me) {}
	virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) { return rayResult.m_collisionObject == m_me ? btScalar(1.0) : ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace); }
protected:
	btRigidBody* m_me;
};

void jump(game_entity *ball, btDynamicsWorld *world) {
	// Jumping
	btTransform ballTransform;
	ball->motionState->getWorldTransform(ballTransform);
	btVector3 pos = ballTransform.getOrigin(), btFrom(pos.x(), pos.y(), pos.z()), btTo(pos.x(), pos.y() - 1, pos.z());
	ClosestNotMeRayResultCallback res(btFrom, btTo, ball->body);
	world->rayTest(btFrom, btTo, res);
	if (res.hasHit()){
		ball->body->applyCentralImpulse(btVector3(0.0f, 2.5f * 100, 0.f));
		cout << "jump" << endl;
	}

	btTransform xform;
	ball->motionState->getWorldTransform(xform);
	// btVector3 down = -xform.getBasis()[1];
	btVector3 down = ballTransform.getOrigin();
	// cout << down.getX() << down.getY() << down.getZ() << endl;
	// down.normalize();
	btScalar halfHeight = btScalar(5) / 2;
	btVector3 from(xform.getOrigin()), to(from + down * halfHeight * btScalar(1.1));
	ClosestNotMeRayResultCallback rayCallback(from, to, ball->body);
	rayCallback.m_closestHitFraction = 1.0;
	world->rayTest(from, to, rayCallback);
	if (rayCallback.hasHit())
	{
		if (rayCallback.m_closestHitFraction < btScalar(1.0)) {
			cout << "jump" << endl;
			btTransform xform;
			ball->motionState->getWorldTransform(xform);
			// btVector3 up = xform.getBasis()[1];
			// up.normalize();
			btScalar magnitude = (btScalar(1.0) / ball->body->getInvMass()) * btScalar(8.0);
			// ball->body->applyCentralImpulse(up * magnitude);
			ball->body->applyCentralImpulse(btVector3(0.0f, 10 * magnitude, 0.f));
		}
	}
}

inline void printVector(float *value) {
	cout << "x: " << value[0] << " y: " << value[1] << " z: " << value[2] << " w: " << value[3] << endl;
}

inline void printMatrix(float *matrixValue) {
	cout << "-------------" << endl;
	for (int i = 0; i < 4; i++) {
		// cout << matrixValue[i * 4 + 0] << " " << matrixValue[i * 4 + 1] << " " << matrixValue[i * 4 + 2] << " " << matrixValue[i * 4 + 3] << endl;
		printf("|%2.0f|%2.0f|%2.0f|%2.0f|\n", matrixValue[i * 4 + 0], matrixValue[i * 4 + 1], matrixValue[i * 4 + 2], matrixValue[i * 4 + 3]);
		cout << "-------------" << endl;
	}
}

int main(int argc, char *argv[]) {
	/*obj_model *obj = load_obj_model(RESOURCE_DIR "cs_office/cs_office.obj");
	for (unsigned int i = 0; i < obj->numParts; i++) {
	obj_model_part *part = obj->parts[i];

	for (unsigned int j = 0; j < part->vertexCount; j += 8) {
	cout << part->vertices[j] << " " << part->vertices[j + 1] << " " << part->vertices[j + 2] << " " << part->vertices[j + 3] << " " << part->vertices[j + 4] << " " << part->vertices[j + 5] << " " << part->vertices[j + 6] << " " << part->vertices[j + 7] << endl;
	}
	}
	destroy_obj_model(obj);

	system("pause");*/
	// return 0;

	ALIGN(16) float value[4], matrixValue[16];
	VEC v1 = VectorSet(10, 3, 5, 0),
		v2 = VectorReplicate(10);
	// cout << "||v1|| = " << VectorLength(v1) << endl;
	// v1 = VectorMultiply(v1, v2);
	// v1 = VectorNormalize(v1);

	MAT m1 = MatrixIdentity();
	m1 = MatrixSet(
		3, 4, 7, 1,
		0, 3, 12, 14,
		5, 6, 23, 11,
		8, 9, 2, 0);
	printMatrix(MatrixGet(matrixValue, m1));
	m1 = MatrixInverse(&m1);
	printMatrix(MatrixGet(matrixValue, m1));
	/*cout << "--------" << endl;
	m1 = MatrixTranspose(&m1);
	printMatrix(MatrixGet(matrixValue, m1));*/

	// printVector(VectorGet(value, v1));

	/* First, initialize SDL's video subsystem. */
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return 1;
	}
	SDL_SetRelativeMouseMode(SDL_TRUE); // Capture mouse and use relative coordinates
	SDL_Window *win = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (int)WINDOW_WIDTH, (int)WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
	if (win == 0) {
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}
	SDL_GLContext glcontext = SDL_GL_CreateContext(win);
	GLenum err = glewInit();
	if (err != GLEW_OK) std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
	cout << "OpenGL vendor: " << glGetString(GL_VENDOR) << ", renderer: " << glGetString(GL_RENDERER) << ", version: " << glGetString(GL_VERSION) << endl << "Using GLEW " << glewGetString(GLEW_VERSION) << endl;
	cout << "OpenGL version: " << OGLV << " Bullet version: " << BT_BULLET_VERSION << endl;

	// Depth testing
	glDepthFunc(GL_LEQUAL);
	// Alpha blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// Culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	// Texturing
	// glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	VEC position = VectorReplicate(0);
	float yaw = 0.f, pitch = 0.f;
	MAT projection = MatrixPerspective(90.f, WINDOW_WIDTH / WINDOW_HEIGHT, 1.f, 3000.f), view, model = MatrixIdentity();
	ALIGN(16) float vv[4], mv[16];

	const float skyboxVertices[] = { -1, -1, 1, -1, 1, 1, -1, 1 };
	const unsigned int cubeIndices[] = { 0, 1, 2, 0, 2, 3 }; /* Indices for the faces of a Cube. */
	GLuint skyboxTex = setupSkyboxTexture();
	GLuint skyboxProg = create_program(shaders::skyboxVertexSrc, shaders::skyboxFragmentSrc);
	glUseProgram(skyboxProg);
	GLchar *infoLog = getProgramInfoLog(skyboxProg);
	cout << "Skybox program info log: " << infoLog << endl;
	free(infoLog);
	struct attrib skyboxAttribs[] = { { glGetAttribLocation(skyboxProg, "vertex"), 2, 0 }, NULL_ATTRIB };
	struct mesh *skybox = create_mesh(1);
	cout << "Stride: " << calculate_stride(skyboxAttribs) << endl;
	glBindBuffer(GL_ARRAY_BUFFER, skybox->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, skyboxVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skybox->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 6, cubeIndices, GL_STATIC_DRAW);

	// f1::program *phong = new program(shaders::phongVertexSrc, shaders::phongFragmentSrc);
	// cout << "Shader program info log: " << *phong << endl;
	GLuint phong = create_program(shaders::phongVertexSrc, shaders::phongFragmentSrc);
	infoLog = getProgramInfoLog(skyboxProg);
	cout << "Shader program info log: " << infoLog << endl;
	free(infoLog);
	// mesh *mesh = f1::getObjModel("bunny.obj");
	btBvhTriangleMeshShape *groundShape;
	struct model *groundMesh2 = loadMeshUsingAssimp(RESOURCE_DIR "cs_office/cs_office.obj", phong, true, groundShape); // "cube_texture.obj/cube_texture.obj"
	struct model *groundMesh = loadMeshUsingObjLoader(RESOURCE_DIR "cs_office/cs_office.obj", phong);

	// Bullet Physics initialization
	game_world *world = new game_world;
	world->init();
	/* Ground body */
	// btCollisionShape *groundShape = /* new btStaticPlaneShape(btVector3(0, 1, 0), 1) */ new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
	btDefaultMotionState *groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
	game_entity *ground = new game_entity(world->world, 0, groundMotionState, groundShape, btVector3(0, 0, 0));
	ground->body->setFriction(1.0f); // 5.2
	world->add_entity(ground);
	/* Ball body */
	// btCollisionShape* ballShape = new btSphereShape(1);
	btCollisionShape *ballShape = new btBoxShape(btVector3(5, 5, 5));
	btDefaultMotionState* ballMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 50, 0)));
	btScalar mass = 1;
	btVector3 ballInertia(0, 0, 0);
	ballShape->calculateLocalInertia(mass, ballInertia);
	ball = new game_entity(world->world, mass, ballMotionState, ballShape, ballInertia);
	ball->body->setSleepingThresholds(btScalar(.0f), btScalar(.0f));
	ball->body->setAngularFactor(.0f);
	ball->body->setRestitution(0.f);
	ball->body->setCcdMotionThreshold(1.0f);
	ball->body->setCcdSweptSphereRadius(0.2f);
	world->add_entity(ball);

	bool noclip = true;

	bool done = false;
	SDL_Event event;
	const Uint8 *state = SDL_GetKeyboardState(NULL);
	while (!done) {
		Uint32 start_time = SDL_GetTicks();
		while (SDL_PollEvent(&event) != 0) switch (event.type) { case SDL_QUIT: done = true; break; }

		/* Mouse */
		int x, y;
		SDL_GetRelativeMouseState(&x, &y);
		update_camera(&yaw, x * MOUSE_SENSITIVITY, &pitch, y *MOUSE_SENSITIVITY);
		/* Keyboard */
		if (state[SDL_SCANCODE_ESCAPE]) done = true;
		// TODO: optimize
		if (state[SDL_SCANCODE_W]) walk(&position, yaw, MOVEMENT_SPEED, 180, ball->body);
		if (state[SDL_SCANCODE_A]) walk(&position, yaw, MOVEMENT_SPEED, 270, ball->body);
		if (state[SDL_SCANCODE_S]) walk(&position, yaw, MOVEMENT_SPEED, 0, ball->body);
		if (state[SDL_SCANCODE_D]) walk(&position, yaw, MOVEMENT_SPEED, 90, ball->body);
		if (!(state[SDL_SCANCODE_W] || state[SDL_SCANCODE_A] || state[SDL_SCANCODE_S] || state[SDL_SCANCODE_D])) {
			ball->body->setLinearVelocity(btVector3(0, ball->body->getLinearVelocity().y(), 0));
		}
		if (state[SDL_SCANCODE_SPACE]) {
			position = VectorAdd(position, VectorSet(0, MOVEMENT_SPEED, 0, 0)); // position.y += MOVEMENT_SPEED;

			jump(ball, world->world);
		}
		if (state[SDL_SCANCODE_LSHIFT]) position = VectorAdd(position, VectorSet(0, -MOVEMENT_SPEED, 0, 0)); // position.y -= MOVEMENT_SPEED;
		if (state[SDL_SCANCODE_C]) noclip = !noclip;
		// Set the view matrix
		btTransform t;
		if (noclip) {
			// VectorGet(vv, position); view = MatrixTranslation(vv[0], vv[1], vv[2]);
			view = MatrixTranslationFromVector(position);
		}
		else {
			ball->body->getMotionState()->getWorldTransform(t);
			// t.getOpenGLMatrix(glm::value_ptr(view));
			t.getOpenGLMatrix(mv);
			view = MatrixLoad(mv);
			/*btTransform ballTransform;
			ball->motionState->getWorldTransform(ballTransform);
			btVector3 pos = ballTransform.getOrigin();
			view = glm::translate(glm::mat4(1.f), glm::vec3(pos.x(), pos.y(), pos.z()));*/
		}
		MAT rotationViewMatrix = MatrixRotationQuaternion(QuaternionRotationRollPitchYaw(pitch, yaw, 0));
		MAT vInv = (MatrixMultiply(&rotationViewMatrix, &view));
		view = MatrixInverse(&vInv);
		MAT modelView = MatrixMultiply(&model, &view), viewProjection = MatrixMultiply(&view, &projection);

		glClearColor(0.5f, 0.3f, 0.8f, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* Draw skybox. */
		glDisable(GL_DEPTH_TEST);
		glUseProgram(skyboxProg);
		glUniformMatrix4fv(glGetUniformLocation(skyboxProg, "invProjection"), 1, GL_FALSE, MatrixGet(mv, MatrixInverse(&projection)));
		glUniformMatrix4fv(glGetUniformLocation(skyboxProg, "trnModelView"), 1, GL_FALSE, MatrixGet(mv, MatrixTranspose(&modelView)));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
		glUniform1i(glGetUniformLocation(skyboxProg, "texture"), 0);
		bind_mesh(skybox, skyboxAttribs, 0, glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, BUFFER_OFFSET(0)););

		/* Draw everything. */
		glEnable(GL_DEPTH_TEST);
		glUseProgram(phong);
		glUniformMatrix4fv(glGetUniformLocation(phong, "p"), 1, GL_FALSE, MatrixGet(mv, projection));
		glUniformMatrix4fv(glGetUniformLocation(phong, "v"), 1, GL_FALSE, MatrixGet(mv, view));
		glUniformMatrix4fv(glGetUniformLocation(phong, "vInv"), 1, GL_FALSE, MatrixGet(mv, vInv));
		VectorGet(vv, position);
		glUniform4f(glGetUniformLocation(phong, "eyeCoords"), vv[0], vv[1], vv[2], 1);

		/* Draw ground. */
		ground->body->getMotionState()->getWorldTransform(t); // Get the transform from Bullet and into 't'
		t.getOpenGLMatrix(mv); // Convert the btTransform into the GLM matrix using 'glm::value_ptr'
		model = MatrixLoad(mv);
		MAT scaleMatrix = MatrixScaling(100, 100, 100);
		// model = MatrixMultiply(&model, &scaleMatrix);
		glUniformMatrix4fv(glGetUniformLocation(phong, "m"), 1, GL_FALSE, MatrixGet(mv, model));
		glUniformMatrix4fv(glGetUniformLocation(phong, "normalMatrix"), 1, GL_FALSE, MatrixGet(mv, MatrixInverse(&modelView)));
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(phong, "tex"), 0);
		for (unsigned int i = 0; i < groundMesh->nodeCount; i++) {
			model_node *node = groundMesh->nodes[i];

			glBindTexture(GL_TEXTURE_2D, node->texture);
			// bind_mesh(node->mesh, node->attributes, node->stride, glDrawElements(GL_TRIANGLES, node->indexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(0)););
			bind_mesh(node->mesh, node->attributes, node->stride, glDrawArrays(GL_TRIANGLES, 0, node->vertexCount););
		}

		// world->world->debugDrawWorld(); // Debug draw the bullet world
		// dynamic_cast<GLDebugDrawer*>(world->debugDraw)->end(MatrixGet(mv, MatrixMultiply(&model, &viewProjection)));

		/* Draw bunny model. */
		/*ball->body->getMotionState()->getWorldTransform(t); // Get the transform from Bullet and into 't'
		t.getOpenGLMatrix(glm::value_ptr(model)); // Convert the btTransform into the GLM matrix using 'glm::value_ptr'
		model = glm::scale(model, vec3(100.f));
		glUniformMatrix4fv(glGetUniformLocation(*prog, "m"), 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(glGetUniformLocation(*prog, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(inverse(model * view)));
		mesh->bind(prog);
		mesh->draw(GL_TRIANGLES, 0, 3 * 4968); // 4968, 3968
		mesh->unbind(prog);
		/ * Draw light. */
		/*model = glm::translate(mat4(1.f), vec3(-2, 2, 0));
		glUniformMatrix4fv(glGetUniformLocation(*prog, "m"), 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(glGetUniformLocation(*prog, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(inverse(model * view)));
		mesh->bind(prog);
		mesh->draw(GL_TRIANGLES, 0, 3 * 4968); // 4968, 3968
		mesh->unbind(prog);*/

		GLenum error;
		while ((error = glGetError()) != 0) cout << "GL error: " << error << endl;/**/
		SDL_GL_SwapWindow(win);
		world->world->stepSimulation(1 / 60.f, 100); // 60 10
		if ((1000 / FPS) > (SDL_GetTicks() - start_time)) SDL_Delay((1000 / FPS) - (SDL_GetTicks() - start_time));
	}

	SDL_HideWindow(win); // Hide the window to make the cleanup time transparent

	delete ground;
	delete ball;
	delete world;

	glDeleteProgram(phong);
	destroy_model(groundMesh2);
	destroy_model(groundMesh);

	glDeleteTextures(1, &skyboxTex);
	glDeleteProgram(skyboxProg);
	destroy_mesh(skybox);

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(win);
	SDL_Quit();

	return 0;
}
