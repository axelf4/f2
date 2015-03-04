#include <iostream>

#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>

#include <glh.h>
#include <vmath.h>
#include <net.h>

#include <SOIL.h>

#include <btBulletDynamicsCommon.h>
#include <entityx/entityx.h>

#include "shaders.h"
#include "GLDebugDrawer.h"
#include "netmessages.pb.h"
#include "server.h"
#include "shared.h"

using namespace game;

// #define WINDOW_TITLE "Point and Click Adventures"
#define WINDOW_TITLE "Call of Duty: Avancerad V\xC3\xA4lf\xC3\xA4rd"
// 640/480, 800/600, 1366/768
#define WINDOW_WIDTH 800.f
#define WINDOW_HEIGHT 600.f
#define FPS 60
#define MOUSE_SENSITIVITY 0.006f

#ifndef RESOURCE_DIR
#define RESOURCE_DIR "resource/"
#endif

using namespace std;

void update_camera(float *yaw, float yaw_amount, float *pitch, float pitch_amount) {
	*yaw -= yaw_amount;
	*pitch -= pitch_amount;
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

inline void printVector(float *value) {
	cout << "x: " << value[0] << " y: " << value[1] << " z: " << value[2] << " w: " << value[3] << endl;
}

inline void printMatrix(float *matrixValue) {
	cout << "-------------" << endl;
	for (int i = 0; i < 4; i++) {
		// cout << matrixValue[i * 4 + 0] << " " << matrixValue[i * 4 + 1] << " " << matrixValue[i * 4 + 2] << " " << matrixValue[i * 4 + 3] << endl;
		// printf("|%2.0f|%2.0f|%2.0f|%2.0f|\n", matrixValue[i * 4 + 0], matrixValue[i * 4 + 1], matrixValue[i * 4 + 2], matrixValue[i * 4 + 3]);
		printf("|%2f|%2f|%2f|%2f|\n", matrixValue[i * 4 + 0], matrixValue[i * 4 + 1], matrixValue[i * 4 + 2], matrixValue[i * 4 + 3]);
		cout << "-------------" << endl;
	}
}

int main(int argc, char *argv[]) {
	GOOGLE_PROTOBUF_VERIFY_VERSION; // Verify that the version of the library that we linked against is compatible with the version of the headers we compiled against
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
	struct model *groundMesh = loadMeshUsingObjLoader(RESOURCE_DIR "cs_office/cs_office.obj", phong, true, load_texture);
	btBvhTriangleMeshShape *groundShape = new btBvhTriangleMeshShape(groundMesh->tiva, true);

	entityx::EntityX entityx;
	std::shared_ptr<game::CollisionSystem> collisionSystem = entityx.systems.add<CollisionSystem>();
	entityx.systems.configure();
	/* Ground body */
	entityx::Entity ground = game::createGround(entityx, groundShape);
	entityx::Entity player = game::createPlayer(entityx);
	btRigidBody *ballBody = player.component<RigidBody>()->body;

	net_initialize(); // Initialize the net component
	game::Server *server = new game::Server;

	game::PacketBase packet;
	game::ClientLoginPacket *clientLogin = new game::ClientLoginPacket;
	clientLogin->set_test("HelloWorld!");
	packet.set_allocated_clientloginpacket(clientLogin);
	packet.set_type(game::PacketBase_Type_ClientLoginPacket);

	peer *client = net_peer_create(0, 1);
	int len = packet.ByteSize() + NET_SEQNO_SIZE;
	unsigned char *buffer = new unsigned char[len];
	packet.SerializeToArray(buffer, len - NET_SEQNO_SIZE);
	struct sockaddr_in sendAddress;
	NET_IP4_ADDR("127.0.0.1", 30000, &sendAddress);
	net_send(client, buffer, len, sendAddress, NET_RELIABLE);
	// net_send(client, buffer, len, net_address("127.0.0.1", 30000), NET_RELIABLE);

	bool noclip = false;

	bool done = false;
	SDL_Event event;
	const Uint8 *state = SDL_GetKeyboardState(NULL);
	while (!done) {
		float dt = 1.f / 60; // I don't feel like writing a proper game loop just now
		Uint32 start_time = SDL_GetTicks();
		while (SDL_PollEvent(&event) != 0) switch (event.type) { case SDL_QUIT: done = true; break; }

		server->update(dt);
		net_update(client);
		unsigned char recvbuf[1024];
		int recvbuflen;
		struct sockaddr_in from;
		while ((recvbuflen = net_receive(client, recvbuf, sizeof recvbuf, &from)) > 0) {
			// cout << "Received: " << recvbuflen << " bytes or '" << (char *)recvbuf << "'." << endl;

			game::PacketBase packet;
			packet.ParseFromArray(recvbuf, recvbuflen - NET_SEQNO_SIZE);
			game::PacketBase_Type type = packet.type();
			if (type == game::PacketBase_Type_GAMEST) {
				// cout << "Oh oh a new gamestate incoming" << endl;
				game::gamest gamest = packet.gamest();
				for (int i = 0; i < gamest.entity_size(); i++) {
					game::entity_state entity = gamest.entity(i);
					game::vec3 origin = entity.origin();
					btTransform transform = ballBody->getWorldTransform();
					transform.setOrigin(btVector3(origin.x(), origin.y(), origin.z()));
					ballBody->setWorldTransform(transform);
				}
				
			}
		}

		/* Mouse */
		int x, y;
		SDL_GetRelativeMouseState(&x, &y);
		update_camera(&yaw, x * MOUSE_SENSITIVITY, &pitch, y *MOUSE_SENSITIVITY);
		/* Keyboard */
		if (state[SDL_SCANCODE_ESCAPE]) done = true;
		if (state[SDL_SCANCODE_W]) walk(&position, yaw, MOVEMENT_SPEED, 180, ballBody);
		if (state[SDL_SCANCODE_A]) walk(&position, yaw, MOVEMENT_SPEED, 270, ballBody);
		if (state[SDL_SCANCODE_S]) walk(&position, yaw, MOVEMENT_SPEED, 0, ballBody);
		if (state[SDL_SCANCODE_D]) walk(&position, yaw, MOVEMENT_SPEED, 90, ballBody);
		if (!(state[SDL_SCANCODE_W] || state[SDL_SCANCODE_A] || state[SDL_SCANCODE_S] || state[SDL_SCANCODE_D])) {
			ballBody->setLinearVelocity(btVector3(0, ballBody->getLinearVelocity().y(), 0));
		}
		if (state[SDL_SCANCODE_SPACE]) {
			position = VectorAdd(position, VectorSet(0, MOVEMENT_SPEED, 0, 0)); // position.y += MOVEMENT_SPEED;

			jump(player.component<RigidBody>().get(), collisionSystem->world);
		}
		if (state[SDL_SCANCODE_LSHIFT]) position = VectorAdd(position, VectorSet(0, -MOVEMENT_SPEED, 0, 0)); // position.y -= MOVEMENT_SPEED;

		game::PacketBase movePacket;
		game::usercmd *moveMsg = movePacket.mutable_usercmd();
		moveMsg->set_seqno(0);
		game::vec3 *viewangles = moveMsg->mutable_viewangles();
		viewangles->set_x(pitch);
		viewangles->set_y(yaw);
		viewangles->set_z(0);

		moveMsg->set_w(state[SDL_SCANCODE_W]);
		moveMsg->set_a(state[SDL_SCANCODE_A]);
		moveMsg->set_s(state[SDL_SCANCODE_S]);
		moveMsg->set_d(state[SDL_SCANCODE_D]);
		bool space = state[SDL_SCANCODE_SPACE];
		moveMsg->set_space(space);
		sockaddr_in address;
		NET_IP4_ADDR("127.0.0.1", 30000, &address);
		sendPacket(client, address, movePacket, game::PacketBase_Type_USERCMD, space ? NET_RELIABLE : NET_UNRELIABLE);

		if (state[SDL_SCANCODE_C]) noclip = !noclip;
		// Set the view matrix
		btTransform t;
		if (noclip) {
			// VectorGet(vv, position); view = MatrixTranslation(vv[0], vv[1], vv[2]);
			view = MatrixTranslationFromVector(position);
		}
		else {
			ballBody->getMotionState()->getWorldTransform(t);
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
		ground.component<RigidBody>()->body->getMotionState()->getWorldTransform(t); // Get the transform from Bullet and into 't'
		t.getOpenGLMatrix(mv); // Convert the btTransform into the GLM matrix using 'glm::value_ptr'
		model = MatrixLoad(mv);
		// MAT scaleMatrix = MatrixScaling(100, 100, 100);
		// model = MatrixMultiply(&model, &scaleMatrix);
		glUniformMatrix4fv(glGetUniformLocation(phong, "m"), 1, GL_FALSE, MatrixGet(mv, model));
		glUniformMatrix4fv(glGetUniformLocation(phong, "normalMatrix"), 1, GL_FALSE, MatrixGet(mv, MatrixInverse(&modelView)));
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(phong, "tex"), 0);
		for (unsigned int i = 0; i < groundMesh->nodeCount; i++) {
			model_node *node = groundMesh->nodes[i];

			glBindTexture(GL_TEXTURE_2D, node->texture);
			bind_mesh(node->mesh, node->attributes, node->stride, glDrawElements(GL_TRIANGLES, node->indexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(0)););
			// bind_mesh(node->mesh, node->attributes, node->stride, glDrawArrays(GL_TRIANGLES, 0, node->vertexCount););
		}

		// world->world->debugDrawWorld(); // Debug draw the bullet world
		// dynamic_cast<GLDebugDrawer*>(world->debugDraw)->end(MatrixGet(mv, MatrixMultiply(&model, &viewProjection)));

		GLenum error;
		while ((error = glGetError()) != 0) cout << "GL error: " << error << endl;/**/
		SDL_GL_SwapWindow(win);
		entityx.systems.update_all(dt);
		if ((1000 / FPS) > (SDL_GetTicks() - start_time)) SDL_Delay((1000 / FPS) - (SDL_GetTicks() - start_time));
	}

	SDL_HideWindow(win); // Hide the window to make the cleanup time transparent

	delete server;
	net_peer_dispose(client);
	net_deinitialize();

	glDeleteProgram(phong);
	destroy_model(groundMesh);

	glDeleteTextures(1, &skyboxTex);
	glDeleteProgram(skyboxProg);
	destroy_mesh(skybox);

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(win);
	SDL_Quit();

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}
