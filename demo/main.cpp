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
#define WINDOW_TITLE "Call of Duty: Avancerad V\xC3\xA4lf\xC3\xA4rd v0.0.1 \"Sex and Candy\" Release"
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

GLuint setupSkyboxTexture() {
	/* load image into a new OpenGL cube map, forcing RGB */
	GLuint cubemap = SOIL_load_OGL_single_cubemap(RESOURCE_DIR "skybox.png", "SNDUWE", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return cubemap;
}

struct GBuffer {
	enum GBUFFER_TEXTURE_TYPE {
		GBUFFER_TEXTURE_TYPE_POSITION,
		GBUFFER_TEXTURE_TYPE_DIFFUSE,
		GBUFFER_TEXTURE_TYPE_NORMAL,
		GBUFFER_NUM_TEXTURES
	};

	GLuint fbo,
		textures[GBUFFER_NUM_TEXTURES],
		depthTexture;

	GBuffer() {}

	~GBuffer() {
		//Delete resources
		glDeleteTextures(1, &depthTexture);
		glDeleteTextures(GBUFFER_NUM_TEXTURES, textures);
		//Bind 0, which means render to back buffer, as a result, fb is unbound
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffersEXT(1, &fbo);
	}

	bool Init(unsigned int WindowWidth, unsigned int WindowHeight) {
		// TODO we only need diffuse normal and depth!

		// Create the FBO
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		// Create the gbuffer textures
		glGenTextures(GBUFFER_NUM_TEXTURES, textures);
		for (unsigned int i = 0; i < GBUFFER_NUM_TEXTURES; i++) {
			glBindTexture(GL_TEXTURE_2D, textures[i]);
			GLint internalFormat;
			if (i == GBUFFER_TEXTURE_TYPE_DIFFUSE) internalFormat = GL_RGB;
			else if (i == GBUFFER_TEXTURE_TYPE_NORMAL) internalFormat = GL_RGB16F;
			else if (i == GBUFFER_TEXTURE_TYPE_POSITION) internalFormat = GL_RGB32F;
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WindowWidth, WindowHeight, 0, GL_RGB, GL_FLOAT, NULL);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, textures[i], 0);
		}

		// depth
		glGenTextures(1, &depthTexture);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		/*glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, WindowWidth, WindowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);*/
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, WindowWidth, WindowHeight, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

		GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, DrawBuffers);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			printf("FB error, status: 0x%x\n", status);
			return false;
		}

		// restore default FBO
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		return true;
	}

	void BindForReadingTextures() {
		for (unsigned int i = 0; i < GBUFFER_NUM_TEXTURES; i++) {
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, textures[GBUFFER_TEXTURE_TYPE_POSITION + i]);
		}
		glActiveTexture(GL_TEXTURE0 + GBUFFER_NUM_TEXTURES);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
	}
};

float lerp(float v0, float v1, float t) {
	return v0 + t * (v1 - v0);
	// return (1 - t) * v0 + t * v1;
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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

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
	// glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
	glUseProgram(skyboxProg); // glUniform1i(glGetUniformLocation(skyboxProg, "texture"), 0);
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
	// GLuint phong = create_program(shaders::phongVertexSrc, shaders::phongFragmentSrc);
	GLuint phong = create_program(read_file(RESOURCE_DIR "geometry_pass.vs"), read_file(RESOURCE_DIR "geometry_pass.fs"));
	infoLog = getProgramInfoLog(skyboxProg);
	cout << "Shader program info log: " << infoLog << endl;
	free(infoLog);
	struct model *groundMesh = loadMeshUsingObjLoader(RESOURCE_DIR "cs_office/cs_office.obj", phong, true, load_texture); // cs_office/cs_office.obj
	btBvhTriangleMeshShape *groundShape = new btBvhTriangleMeshShape(groundMesh->tiva, true);

	GBuffer *gbuffer = new GBuffer();
	gbuffer->Init(WINDOW_WIDTH, WINDOW_HEIGHT);
	GLuint lightpass = create_program(read_file(RESOURCE_DIR "light_pass.vs"), read_file(RESOURCE_DIR "dir_light_pass.fs"));
	infoLog = getProgramInfoLog(skyboxProg);
	cout << "Light_pass program info log: " << infoLog << endl;
	free(infoLog);

	entityx::EntityX entityx;
	std::shared_ptr<game::CollisionSystem> collisionSystem = entityx.systems.add<CollisionSystem>();
	entityx.systems.configure();
	/* Ground body */
	entityx::Entity ground = game::createGround(entityx, groundShape);
	entityx::Entity player = game::createPlayer(entityx);
	btRigidBody *ballBody = player.component<RigidBody>()->body;

	game::usercmd commands[256];
	unsigned int lastAdded = 0, lastReceived = 0;
	btVector3 lastPosition(0, 0, 0);

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

	bool noclip = true;

	bool done = false;
	SDL_Event event;
	const Uint8 *state = SDL_GetKeyboardState(NULL);
	while (!done) {
		float dt = 1.f / FPS; // (0-1) I don't feel like writing a proper game loop just now
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
				unsigned int seqno = gamest.seqno();
				if (seqno <= lastReceived) continue;
				lastReceived = seqno;

				for (int i = 0; i < gamest.entity_size(); i++) {
					game::entity_state entity = gamest.entity(i);
					game::vec3 origin = entity.origin();

					btTransform transform = ballBody->getWorldTransform();
					btVector3 oldOrigin = transform.getOrigin();
					// transform.setOrigin(btVector3(origin.x(), origin.y(), origin.z()));
					transform.setOrigin(btVector3(
						lerp(oldOrigin.x(), origin.x(), dt),
						lerp(oldOrigin.y(), origin.y(), dt),
						lerp(oldOrigin.z(), origin.z(), dt)));
					ballBody->setWorldTransform(transform);

					game::vec3 velocity = entity.velocity();
					ballBody->setLinearVelocity(btVector3(velocity.x(), velocity.y(), velocity.z()));
				}

				if (lastAdded > seqno) {
					int nums = lastAdded - seqno;
					int i = seqno + 1;
					do {
						applyUsercmd(commands[i % 256], player.component<RigidBody>().get(), collisionSystem->world, 0);
					} while (i++ < lastAdded);
				}
			}
		}

		/* Mouse */
		int x, y;
		SDL_GetRelativeMouseState(&x, &y);
		update_camera(&yaw, x * MOUSE_SENSITIVITY, &pitch, y *MOUSE_SENSITIVITY);
		/* Keyboard */
		if (state[SDL_SCANCODE_ESCAPE]) done = true;
		/*if (state[SDL_SCANCODE_W]) walk(&position, yaw, MOVEMENT_SPEED, 180, ballBody);
		if (state[SDL_SCANCODE_A]) walk(&position, yaw, MOVEMENT_SPEED, 270, ballBody);
		if (state[SDL_SCANCODE_S]) walk(&position, yaw, MOVEMENT_SPEED, 0, ballBody);
		if (state[SDL_SCANCODE_D]) walk(&position, yaw, MOVEMENT_SPEED, 90, ballBody);
		if (!(state[SDL_SCANCODE_W] || state[SDL_SCANCODE_A] || state[SDL_SCANCODE_S] || state[SDL_SCANCODE_D])) {
		ballBody->setLinearVelocity(btVector3(0, ballBody->getLinearVelocity().y(), 0));
		}
		if (state[SDL_SCANCODE_SPACE]) {
		position = VectorAdd(position, VectorSet(0, MOVEMENT_SPEED, 0, 0)); // position.y += MOVEMENT_SPEED;

		jump(player.component<RigidBody>().get(), collisionSystem->world);
		}*/
		if (state[SDL_SCANCODE_LSHIFT]) position = VectorAdd(position, VectorSet(0, -MOVEMENT_SPEED, 0, 0)); // position.y -= MOVEMENT_SPEED;

		game::PacketBase movePacket;
		game::usercmd *usercmd = movePacket.mutable_usercmd();
		game::vec3 *viewangles = usercmd->mutable_viewangles();
		viewangles->set_x(pitch);
		viewangles->set_y(yaw);
		viewangles->set_z(0);

		usercmd->set_w(!!state[SDL_SCANCODE_W]);
		usercmd->set_a(!!state[SDL_SCANCODE_A]);
		usercmd->set_s(!!state[SDL_SCANCODE_S]);
		usercmd->set_d(!!state[SDL_SCANCODE_D]);
		bool space = !!state[SDL_SCANCODE_SPACE];
		usercmd->set_space(space);

		if (usercmd->w() || usercmd->a() || usercmd->s() || usercmd->d() || usercmd->space()) {
			commands[lastAdded++ % 256] = *usercmd;
		}
		usercmd->set_seqno(lastAdded);
		applyUsercmd(*usercmd, player.component<RigidBody>().get(), collisionSystem->world, &position);

		sockaddr_in address;
		NET_IP4_ADDR("127.0.0.1", 30000, &address);
		sendPacket(client, address, movePacket, game::PacketBase_Type_USERCMD, space ? NET_RELIABLE : NET_UNRELIABLE);

		if (state[SDL_SCANCODE_C]) noclip = !noclip;
		// Set the view matrix
		if (noclip) {
			// VectorGet(vv, position); view = MatrixTranslation(vv[0], vv[1], vv[2]);
			view = MatrixTranslationFromVector(position);
		}
		else {
			btTransform t;
			ballBody->getMotionState()->getWorldTransform(t);

			/*btVector3 origin = t.getOrigin();
			for (int i = 0; i < 20; i++) {
			lastPosition = btVector3(
			lerp(lastPosition.x(), origin.x(), dt),
			lerp(lastPosition.y(), origin.y(), dt),
			lerp(lastPosition.z(), origin.z(), dt));
			}
			if (!state[SDL_SCANCODE_E]) t.setOrigin(lastPosition);*/

			t.getOpenGLMatrix(mv);
			view = MatrixLoad(mv);
		}
		MAT rotationViewMatrix = MatrixRotationQuaternion(QuaternionRotationRollPitchYaw(pitch, yaw, 0));
		MAT vInv = (MatrixMultiply(&rotationViewMatrix, &view));
		view = MatrixInverse(&vInv);
		MAT modelView = MatrixMultiply(&model, &view), viewProjection = MatrixMultiply(&view, &projection);

		/* Draw everything. */
		// First pass (geometry)
		glBindFramebuffer(GL_FRAMEBUFFER, gbuffer->fbo);
		glDepthMask(GL_TRUE); // Only the geometry pass updates the depth buffer

		/*glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_NEVER, 1, 0xFF); // never pass stencil test
		glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);  // replace stencil buffer values to ref=1
		glStencilMask(0xFF); // stencil buffer free to write*/

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDisable(GL_BLEND);

		glUseProgram(phong);
		glUniformMatrix4fv(glGetUniformLocation(phong, "p"), 1, GL_FALSE, MatrixGet(mv, projection));
		glUniformMatrix4fv(glGetUniformLocation(phong, "v"), 1, GL_FALSE, MatrixGet(mv, view));

		/* Draw ground. */
		btTransform t;
		ground.component<RigidBody>()->body->getMotionState()->getWorldTransform(t); // Get the transform from Bullet and into 't'
		t.getOpenGLMatrix(mv); // Convert the btTransform into the GLM matrix using 'glm::value_ptr'
		model = MatrixLoad(mv);
		glUniformMatrix4fv(glGetUniformLocation(phong, "m"), 1, GL_FALSE, MatrixGet(mv, model));
		// glUniformMatrix4fv(glGetUniformLocation(phong, "normalMatrix"), 1, GL_FALSE, MatrixGet(mv, MatrixInverse(&modelView)));
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(phong, "gColorMap"), 0);
		for (unsigned int i = 0; i < groundMesh->nodeCount; i++) {
			model_node *node = groundMesh->nodes[i];

			glBindTexture(GL_TEXTURE_2D, node->texture);
			bind_mesh(node->mesh, node->attributes, node->stride, glDrawElements(GL_TRIANGLES, node->indexCount, GL_UNSIGNED_INT, BUFFER_OFFSET(0)););
			// bind_mesh(node->mesh, node->attributes, node->stride, glDrawArrays(GL_TRIANGLES, 0, node->vertexCount););
		}

		// Second pass (lightning)
		// When we get here the depth buffer is already populated and the stencil pass
		// depends on it, but it does not write to it.
		// glDepthMask(GL_FALSE);
		// glStencilMask(0x00);
		// glDisable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS); // GL_DEPTH_TEST needs to be enabled for depth to be stored

		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0); // Render to back buffer
		// glDrawBuffer(GL_BACK);

		glClearColor(0.f, 0.f, 0.f, 0.f);
		// glClearDepth(0.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		gbuffer->BindForReadingTextures();

		// directional light
		glUseProgram(lightpass);
		VectorGet(vv, position);
		glUniform3f(glGetUniformLocation(lightpass, "eyeCoords"), vv[0], vv[1], vv[2]);
		glUniformMatrix4fv(glGetUniformLocation(lightpass, "mvp"), 1, GL_FALSE, MatrixGet(mv, MatrixIdentity()));
		glUniform1i(glGetUniformLocation(lightpass, "gPositionMap"), GBuffer::GBUFFER_TEXTURE_TYPE_POSITION);
		glUniform1i(glGetUniformLocation(lightpass, "gColorMap"), GBuffer::GBUFFER_TEXTURE_TYPE_DIFFUSE);
		glUniform1i(glGetUniformLocation(lightpass, "gNormalMap"), GBuffer::GBUFFER_TEXTURE_TYPE_NORMAL);
		glUniform1i(glGetUniformLocation(lightpass, "depthMap"), GBuffer::GBUFFER_NUM_TEXTURES);
		glUniform2f(glGetUniformLocation(lightpass, "gScreenSize"), WINDOW_WIDTH, WINDOW_HEIGHT);
		glUniformMatrix4fv(glGetUniformLocation(lightpass, "vInv"), 1, GL_FALSE, MatrixGet(mv, vInv));
		bind_mesh(skybox, skyboxAttribs, 0, glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, BUFFER_OFFSET(0)););
		/***/

		glDisable(GL_BLEND);
		glDepthFunc(GL_LEQUAL); // GEQUAL
		// glStencilFunc(GL_EQUAL, 1, 0xFF); // stencil test: only pass stencil test at stencilValue == 1 (Assuming depth test would pass.) and write actual content to depth and color buffer only at stencil shape locations.
		/* Draw skybox. */
		glUseProgram(skyboxProg);
		glUniformMatrix4fv(glGetUniformLocation(skyboxProg, "invProjection"), 1, GL_FALSE, MatrixGet(mv, MatrixInverse(&projection)));
		glUniformMatrix4fv(glGetUniformLocation(skyboxProg, "trnModelView"), 1, GL_FALSE, MatrixGet(mv, MatrixTranspose(&modelView)));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
		bind_mesh(skybox, skyboxAttribs, 0, glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, BUFFER_OFFSET(0)););

		// collisionSystem->world->debugDrawWorld(); // Debug draw the bullet world
		// dynamic_cast<GLDebugDrawer*>(collisionSystem->debugDraw)->end(MatrixGet(mv, MatrixMultiply(&model, &viewProjection)));

		GLenum error;
		while ((error = glGetError()) != 0) {
			cout << "GL error: " << error << endl;
		}/**/
		SDL_GL_SwapWindow(win);
		entityx.systems.update_all(dt);
		if ((1000 / FPS) > (SDL_GetTicks() - start_time)) SDL_Delay((1000 / FPS) - (SDL_GetTicks() - start_time));
	}

	SDL_HideWindow(win); // Hide the window to make the cleanup time transparent

	delete server;
	net_peer_dispose(client);
	net_deinitialize();

	glDeleteProgram(lightpass);
	glDeleteProgram(phong);
	glDeleteProgram(skyboxProg);

	glDeleteTextures(1, &skyboxTex);
	destroy_mesh(skybox);
	destroy_model(groundMesh);

	delete gbuffer;

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(win);
	SDL_Quit();

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}
