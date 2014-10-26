#include <Windows.h>
#include <iostream>
#include "sqr.h"
#include "SDL.h"
#include <GL\glew.h>
#include <SDL_opengl.h>
#include <SOIL.h>
#include <glm/mat4x4.hpp>

#include <btBulletDynamicsCommon.h>

/* assimp include files. These three are usually needed. */
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace std;

int main(int argc, char **argv) {
	/* First, initialize SDL's video subsystem. */
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return 1;
	}

	SDL_Window *win = SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL);
	if (win == nullptr) {
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}
	SDL_GLContext glcontext = SDL_GL_CreateContext(win);
	GLenum err = glewInit();
	if (err != GLEW_OK) cerr << "Error: " << glewGetErrorString(err) << endl;


	SOIL_load_OGL_texture("", 0, 0, 0);
	glActiveTexture(GL_TEXTURE0);

	btBroadphaseInterface *broadphase = new btDbvtBroadphase();
	delete broadphase;

	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFile("", // aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType);

	SDL_GL_DeleteContext(glcontext);
	SDL_Quit();

	std::cout << sqr(2) << std::endl;

	system("pause");
	return 0;
}