#ifndef GLH_H
#define GLH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <GL/glew.h>

// TODO let this be the API and create multiple implementations for the different OpenGL versions. And link to them as different DLLs at runtime
// TODO implement doxygen

/** The version of OpenGL. */
#define OGLV (atof((const GLchar*) glGetString(GL_VERSION)))

// TODO remove "#version 110\n" from GLSL for it to be ALOT more flexible
#define GLSL(...) "#version 120\n" #__VA_ARGS__

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#ifdef _DEBUG
#define GL_CHECK(stmt) do { stmt; GLenum err = glGetError(); if (err != GL_NO_ERROR) { printf("OpenGL error %08x, at %s:%i - for %s\n", err, fname, line, #stmt); abort(); } } while(0)
#else
#define GL_CHECK(stmt) stmt
#endif

/** ATTRIBUTES */

// TODO hashtable for attrib location in program macro lookup, DON't do this anymore since the location is stored in attrib struct
struct attrib2 {
	GLint location;
	GLint size;
	int offset;
};

#ifndef __cplusplus
#define NULL_ATTRIB ((struct attrib2) { 0, 0, 0 })
#else
#define NULL_ATTRIB (attrib2 { 0, 0, 0 })
#endif

/* SHADER PROGRAM */

// struct program { GLuint id; }; // maybe dont use struct as the shaders gets deleted immediatlelty after linking leaving only the id which doesn't need to be stored in struct

const char *read_file(const char *filename);

// TODO add support for geometry shaders
/** Links the program. */
GLuint create_program(const GLchar *vertexShader, const GLchar *fragmentShader);

GLuint compile_shader(GLenum type, const GLchar *source, GLint *status);

/** Need to free return value. */
GLchar * getProgramInfoLog(GLuint program);

/* MESH */

struct mesh2 { GLuint vbo, ibo; };

struct mesh2 * create_mesh(int indexed /* = GL_FALSE */);

#define bind_mesh(mesh, attributes, stride, stmt) \
	do { \
		glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo); \
		int i; \
		for (i = 0; attributes[i].size != 0; i++) { \
			attrib2 attribute = attributes[i]; \
			glEnableVertexAttribArray(attribute.location); \
			glVertexAttribPointer(attribute.location, attribute.size, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(attribute.offset)); \
		} \
		if (mesh->ibo != 0) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo); \
		stmt \
		for (; i > 0; i--) glDisableVertexAttribArray(attributes[i].location); \
	} while(0)

GLsizei calculate_stride(struct attrib2 *attributes);

// sGLsizei calculate_stride_count, calculate_stride_i;
#define calculate_stride2(attributes) ( \
	(calculate_stride_count = 0), \
	for(i=0;attributes[i].size != 0; i++) { \
		(attributes)[i]->offset = count; \
		calculate_stride_count += sizeof(float)* (attributes)[i]->size; \
	}, calculate_stride_count \
)

void destroy_mesh(struct mesh2 *mesh);

#ifdef __cplusplus
}
#endif

#endif
