#include <iostream>
#include <stdlib.h>

#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>

#include "f1.h"
#include "glh.h"
#include "vbo.h"

#include "shaders.h"

#define GLM_FORCE_RADIANS
#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SOIL.h>
// #include "obj_loader.h"
/* assimp include files. These three are usually needed. */
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// #include <btBulletDynamicsCommon.h>

// 640/480, 800/600, 1366/768
#define WINDOW_WIDTH 800.f
#define WINDOW_HEIGHT 600.f
#define FPS 60
#define MOUSE_SENSITIVITY 0.006f
#define MOVEMENT_SPEED (0.2f * 50)
#define RESOURCE_DIR "resource/"

using namespace std;
using namespace f1;
using namespace glm;

void update_camera(float *yaw, float yaw_amount, float *pitch, float pitch_amount) {
	*yaw -= yaw_amount;
	*pitch -= pitch_amount;
}

void walk(glm::vec3 *pos, float yaw, float distance, float direction) {
	pos->x += distance * (float)sin(degreesToRadians(yaw + direction));
	pos->z += distance * (float)cos(degreesToRadians(yaw + direction));
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
	f1::mesh *mesh;
	GLsizei vertexCount;
	GLsizei indexCount;
};

struct model {
	model_node **nodes;
	size_t nodeCount;
};

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

model * loadMeshUsingAssimp(const char *filename) {
	unsigned int i;
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

	GLuint *textures = new GLuint[scene->mNumMaterials];
	for (i = 0; i < scene->mNumMaterials; i++) {
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

	for (i = 0; i < scene->mNumMeshes; i++) {
		const struct aiMesh *mesh = scene->mMeshes[i];
		model_node *node = model->nodes[i] = new model_node;

		unsigned int vertexCount = node->vertexCount = mesh->mNumVertices * (mesh->HasPositions() * 3 + mesh->HasTextureCoords(0) * 2 + mesh->HasNormals() * 3);
		float *vertices = new float[vertexCount];
		for (unsigned int i = 0, j = 0; i < mesh->mNumVertices; i++) {
			if (mesh->HasPositions()) {
				vertices[j++] = mesh->mVertices[i].x;
				vertices[j++] = mesh->mVertices[i].y;
				vertices[j++] = mesh->mVertices[i].z;
			}
			if (mesh->HasTextureCoords(0)) {
				vertices[j++] = mesh->mTextureCoords[0][i].x;
				vertices[j++] = mesh->mTextureCoords[0][i].y;
			}
			if (mesh->HasNormals()) {
				vertices[j++] = mesh->mNormals[i].x;
				vertices[j++] = mesh->mNormals[i].y;
				vertices[j++] = mesh->mNormals[i].z;
			}
		}
		unsigned int indexCount = node->indexCount = mesh->mNumFaces * 3;
		unsigned int *indices = new unsigned int[indexCount];
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			const aiFace &face = mesh->mFaces[i]; // Assume that faces're triangles; 3 indices.
			indices[i * 3 + 0] = face.mIndices[0];
			indices[i * 3 + 1] = face.mIndices[1];
			indices[i * 3 + 2] = face.mIndices[2];
		}

		attrib *attributes = new attrib[1 + mesh->HasTextureCoords(0) + mesh->HasNormals()];
		size_t k = 0;
		if (mesh->HasPositions()) attributes[k++] = { "vertex", 3, 0 };
		if (mesh->HasTextureCoords(0)) attributes[k++] = { "texCoord", 2, 0 };
		if (mesh->HasNormals()) attributes[k++] = { "normal", 3, 0 };
		node->mesh = new f1::mesh(true, attributes, k);
		node->mesh->getVertices()->bind();
		node->mesh->getVertices()->setVertices(sizeof(float) * vertexCount, vertices);
		delete[] vertices;
		node->mesh->getIndices()->bind();
		node->mesh->getIndices()->setIndices(sizeof(unsigned int) * indexCount, indices);
		node->texture = textures[mesh->mMaterialIndex];
		delete[] indices;
		
		std::cout << "Loaded mesh " << i << " out of " << scene->mNumMeshes << endl;
	}
	
	std::cout << "Loaded model" << endl;

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
		SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS);
	// glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return cubemap;
}

/*struct game_world;

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

struct game_world {
	btBroadphaseInterface *broadphase;
	btDefaultCollisionConfiguration *collisionConfiguration;
	btCollisionDispatcher *dispatcher;
	btSequentialImpulseConstraintSolver *solver;
	btDiscreteDynamicsWorld *world;

	void init() {
		broadphase = new btDbvtBroadphase();
		collisionConfiguration = new btDefaultCollisionConfiguration();
		dispatcher = new btCollisionDispatcher(collisionConfiguration);
		solver = new btSequentialImpulseConstraintSolver();
		world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
		world->setGravity(btVector3(0, -9.8f, 0));
	}

	~game_world() {
		delete broadphase;
		delete collisionConfiguration;
		delete dispatcher;
		delete solver;
		delete world;
	}

	void add_entity(game_entity *entity) {
		world->addRigidBody(entity->body);
	}
};*/

int main(int argc, char *argv[]) {
	/* First, initialize SDL's video subsystem. */
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return 1;
	}

	SDL_SetRelativeMouseMode(SDL_TRUE); // Capture mouse and use relative coordinates

	SDL_Window *win = SDL_CreateWindow("Point and Click Adventures", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (int)WINDOW_WIDTH, (int)WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
	if (win == 0) {
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}
	SDL_GLContext glcontext = SDL_GL_CreateContext(win);
	GLenum err = glewInit();
	if (err != GLEW_OK) std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
	cout << "OpenGL vendor: " << glGetString(GL_VENDOR) << ", renderer: " << glGetString(GL_RENDERER) << ", version: " << glGetString(GL_VERSION) << endl << "Using GLEW " << glewGetString(GLEW_VERSION) << endl;
	cout << "OpenGL version: " << OGLV << endl;

	// Depth testing
	glDepthFunc(GL_LEQUAL);
	// Alpha blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// Culling
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	// Texturing
	glEnable(GL_TEXTURE_2D);
	// glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glm::vec3 position;
	float yaw = 0.f, pitch = 0.f;
	glm::mat4 projection = glm::perspective(glm::radians(45.f), WINDOW_WIDTH / WINDOW_HEIGHT, 1.f, 3000.f), // 67.f 3000.f
		view, vInv,
		model = glm::scale(glm::mat4(1.f), vec3(1.f));

	/*// Bullet Physics initialization
	game_world *world = new game_world;
	world->init();
	/ * Ground body * /
	btCollisionShape *groundShape = / * new btStaticPlaneShape(btVector3(0, 1, 0), 1) * / new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
	btDefaultMotionState *groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
	game_entity *ground = new game_entity(world->world, 0, groundMotionState, groundShape, btVector3(0, 0, 0));
	world->add_entity(ground);
	/ * Ball body * /
	btCollisionShape* ballShape = new btSphereShape(1);
	btDefaultMotionState* ballMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 50, 0)));
	btScalar mass = 1;
	btVector3 ballInertia(0, 0, 0);
	ballShape->calculateLocalInertia(mass, ballInertia);
	game_entity *ball = new game_entity(world->world, mass, ballMotionState, ballShape, ballInertia);
	world->add_entity(ball);*/

	const unsigned int cubeIndices[] = { 0, 1, 2, 0, 2, 3 }; /* Indices for the faces of a Cube. */
	const float skyboxVertices[] = { -1, -1, 1, -1, 1, 1, -1, 1 };
	GLuint skyboxTex = setupSkyboxTexture();
	program *skyboxProg = new program(shaders::skyboxVertexSrc, shaders::skyboxFragmentSrc);
	cout << "Skybox program info log: " << *skyboxProg << endl;
	attrib *skyboxAttribs = new attrib[1] { { "vertex", 2, 0 } };
	f1::mesh *skybox = new f1::mesh(true, skyboxAttribs, 1);
	skybox->getVertices()->bind();
	skybox->getVertices()->setVertices(sizeof(float) * 2 * 4, skyboxVertices);
	skybox->getIndices()->bind();
	skybox->getIndices()->setIndices(sizeof(unsigned int) * 6, cubeIndices);

	program *phong = new program(shaders::phongVertexSrc, shaders::phongFragmentSrc);
	cout << "Shader program info log: " << *phong << endl;
	// mesh *mesh = f1::getObjModel("bunny.obj");
	struct model *groundMesh = loadMeshUsingAssimp(RESOURCE_DIR "cs_office/cs_office.obj");

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
		if (state[SDL_SCANCODE_W]) walk(&position, yaw, MOVEMENT_SPEED, 180);
		if (state[SDL_SCANCODE_A]) walk(&position, yaw, MOVEMENT_SPEED, 270);
		if (state[SDL_SCANCODE_S]) walk(&position, yaw, MOVEMENT_SPEED, 0);
		if (state[SDL_SCANCODE_D]) walk(&position, yaw, MOVEMENT_SPEED, 90);
		if (state[SDL_SCANCODE_SPACE]) position.y += MOVEMENT_SPEED;
		if (state[SDL_SCANCODE_LSHIFT]) position.y -= MOVEMENT_SPEED;

		glClearColor(0.5f, 0.3f, 0.8f, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		view = glm::translate(glm::mat4(1.f), position);
		view = glm::inverse(vInv = (view * toMat4(normalize(quat(vec3(pitch, yaw, 0))))));

		/* Draw skybox. */
		glDisable(GL_DEPTH_TEST);
		(*skyboxProg)();
		glUniformMatrix4fv(glGetUniformLocation(*skyboxProg, "invProjection"), 1, GL_FALSE, glm::value_ptr(inverse(projection)));
		glUniformMatrix4fv(glGetUniformLocation(*skyboxProg, "trnModelView"), 1, GL_FALSE, glm::value_ptr(transpose(model * view)));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
		glUniform1i(glGetUniformLocation(*skyboxProg, "texture"), 0);
		skybox->bind(skyboxProg);
		skybox->draw(GL_TRIANGLES, 0, 6);
		skybox->unbind(skyboxProg);

		/* Draw everything. */
		glEnable(GL_DEPTH_TEST);
		(*phong)();
		glUniformMatrix4fv(glGetUniformLocation(*phong, "p"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(*phong, "v"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(*phong, "vInv"), 1, GL_FALSE, glm::value_ptr(vInv));
		glUniform4f(glGetUniformLocation(*phong, "eyeCoords"), position.x, position.y, position.z, 1);

		// btTransform t;
		/* Draw ground. */
		// ground->body->getMotionState()->getWorldTransform(t); // Get the transform from Bullet and into 't'
		// t.getOpenGLMatrix(glm::value_ptr(model)); // Convert the btTransform into the GLM matrix using 'glm::value_ptr'
		glUniformMatrix4fv(glGetUniformLocation(*phong, "m"), 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(glGetUniformLocation(*phong, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(inverse(model * view)));
		// set active texture
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(*phong, "tex"), 0);
		FOR(i, groundMesh->nodeCount) {
			model_node *node = groundMesh->nodes[i];

			glBindTexture(GL_TEXTURE_2D, node->texture);

			node->mesh->bind(phong);
			node->mesh->draw(GL_TRIANGLES, 0, node->indexCount); // 36
			node->mesh->unbind(phong);
		}

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
		// world->world->stepSimulation(1 / 60.f, 10);
		if ((1000 / FPS) > (SDL_GetTicks() - start_time)) SDL_Delay((1000 / FPS) - (SDL_GetTicks() - start_time));
	}

	SDL_HideWindow(win);

	/*delete ground;
	delete ball;
	delete world;*/

	for (unsigned int i = 0; i < groundMesh->nodeCount; i++) {
		model_node *node = groundMesh->nodes[i];
		glDeleteTextures(1, &node->texture);
		delete node->mesh;
		delete node;
	}
	delete[] groundMesh->nodes;
	delete groundMesh;
	// delete mesh;
	// delete prog;
	glDeleteTextures(1, &skyboxTex);
	delete skyboxProg;
	delete skybox;

	/* Cleanup */
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(win);
	SDL_Quit();

	return 0;
}
