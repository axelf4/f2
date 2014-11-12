#ifndef GLH_H
#define GLH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <GL/glew.h>

/** The version of OpenGL. */
#define GLV (atof((const GLchar*) glGetString(GL_VERSION)))

// 110, TODO remove "#version 120\n" from GLSL for it to be ALOT more flexible
#define GLSL(...) "#version 120\n" #__VA_ARGS__

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#ifdef _DEBUG
#define GL_CHECK(stmt) do { stmt; GLenum err = glGetError(); if (err != GL_NO_ERROR) { printf("OpenGL error %08x, at %s:%i - for %s\n", err, fname, line, #stmt); abort(); } } while(0)
#else
#define GL_CHECK(stmt) stmt
#endif

extern float OPENGL_VERSION;
void set_opengl_version();

#ifdef __cplusplus
}
#endif

#endif
