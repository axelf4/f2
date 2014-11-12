#ifndef GLH_H
#define GLH_H

#include "f1.h"

#define GL_V (atof((const GLchar*) glGetString(GL_VERSION)))
float OPENGL_VERSION;
void set_opengl_version() { OPENGL_VERSION = GL_V; }

// 110, TODO remove "#version 120\n" from GLSL for it to be ALOT more flexible
#define GLSL(...) "#version 120\n" #__VA_ARGS__

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#ifdef _DEBUG
#define GL_CHECK(stmt) do { stmt; GLenum err = glGetError(); if (err != GL_NO_ERROR) { printf("OpenGL error %08x, at %s:%i - for %s\n", err, fname, line, #stmt); abort(); } } while(0)
#else
#define GL_CHECK(stmt) stmt
#endif

#endif
