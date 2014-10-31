#ifndef GLH_H
#define GLH_H

#include "f1.h"

#define GLSL(src) "#version 120\n" #src

#ifdef _DEBUG
#define GL_CHECK(stmt) do { stmt; GLenum err = glGetError(); if (err != GL_NO_ERROR) { printf("OpenGL error %08x, at %s:%i - for %s\n", err, fname, line, #stmt); abort(); } } while(0)
#else
#define GL_CHECK(stmt) stmt
#endif

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#endif