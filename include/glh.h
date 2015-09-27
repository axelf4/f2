/** Ease-of-use helper library intended for OpenGL.
	\file glh.h
	*/

#ifndef GLH_H
#define GLH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <GL/glew.h>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif
#include <GL/gl.h>
// #include <GL/glext.h>

	// TODO let this be the API and create multiple implementations for the different OpenGL versions. And link to them as different DLLs at runtime
	// TODO implement doxygen

	/** The version of OpenGL as a float. */
#define OGLV (atof((const char*) glGetString(GL_VERSION)))

	/** Returns the GLSL code in a string format. */
#define GLSL(version, ...) "#version " #version "\n" #__VA_ARGS__

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#ifdef _DEBUG
#define GL_CHECK(stmt) do { stmt; GLenum err = glGetError(); if (err != GL_NO_ERROR) { printf("OpenGL error %08x, at %s:%i - for %s\n", err, fname, line, #stmt); abort(); } } while(0)
#else
#define GL_CHECK(stmt) stmt
#endif

	/* ATTRIBUTES */

	struct attrib {
		GLint location;
		GLint size;
		int offset;
	};

#ifndef __cplusplus
#define NULL_ATTRIB ((struct attrib) { 0, 0, 0 })
#else
#define NULL_ATTRIB (attrib { 0, 0, 0 })
#endif

	GLsizei calculate_stride(struct attrib *attributes);

	// sGLsizei calculate_stride_count, calculate_stride_i;
#define calculate_stride2(attributes) ( \
	(calculate_stride_count = 0), \
	for(i=0;attributes[i].size != 0; i++) { \
		(attributes)[i]->offset = count; \
		calculate_stride_count += sizeof(float)* (attributes)[i]->size; \
	}, calculate_stride_count \
)

	/* SHADER PROGRAM */

	// TODO add support for geometry shaders
	/** Links the program. */
	GLuint create_program(const char *vertexShader, const char *fragmentShader);

	GLuint compile_shader(GLenum type, const char *source);

	/* MESH */

	struct mesh { GLuint vbo, ibo; };

	struct mesh * create_mesh(int indexed /* = GL_FALSE */);

	void destroy_mesh(struct mesh *mesh);

#define with_mesh(mesh, attributes, stride, stmt) \
	do { \
		glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo); \
		int i; \
		for (i = 0; attributes[i].size != 0; i++) { \
			struct attrib attribute = attributes[i]; \
			glEnableVertexAttribArray(attribute.location); \
			glVertexAttribPointer(attribute.location, attribute.size, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(attribute.offset)); \
		} \
		if (mesh->ibo != 0) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo); \
		stmt \
		for (; i > 0; i--) glDisableVertexAttribArray(attributes[i].location); \
	} while(0)

#ifdef __cplusplus
}
#endif

#endif
