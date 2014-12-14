#include "glh.h"
#include <stdlib.h>
#include <stdio.h>

const char *read_file(const char *filename) {
	char *buffer = 0;
	long length;
	FILE *f = fopen(filename, "rb");

	if (f) {
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = malloc(length);
		if (buffer) fread(buffer, 1, length, f);
		fclose(f);
	}

	return buffer;
}

GLuint create_program(const GLchar *vertexShader, const GLchar *fragmentShader) {
	GLint vertStatus, fragStatus;
	GLuint vert = compile_shader(GL_VERTEX_SHADER, vertexShader, &vertStatus);
	GLuint frag = compile_shader(GL_FRAGMENT_SHADER, fragmentShader, &fragStatus);
	if (!vertStatus || !fragStatus) return 0; // TODO shaders aren't getting cleaned up in this case

	GLuint id = glCreateProgram();
	if (id == 0) return 0; // TODO shaders aren't getting cleaned up in this case
	glAttachShader(id, vert);
	glAttachShader(id, frag);

	glLinkProgram(id);

	/* We don't need the shaders anymore. */
	glDeleteShader(vert);
	glDeleteShader(vert);

	int linked;
	glGetProgramiv(id, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE) {
		glDeleteProgram(id);
		return 0;
	}
	return id;
}

GLuint compile_shader(GLenum type, const GLchar *source, GLint *status) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);

	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, status); // Check shader compile status
	if (*status == GL_FALSE) {
		GLint i;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &i);
		GLchar *infoLog = malloc(sizeof(GLchar) * i);
		glGetShaderInfoLog(shader, i, NULL, infoLog);
		const char *typeName = type == GL_VERTEX_SHADER ? "GL_VERTEX_SHADER" : (type == GL_FRAGMENT_SHADER ? "GL_FRAGMENT_SHADER" : "unknown");
		fprintf(stderr, "%s: %s\n", typeName, infoLog);
		free(infoLog);
	}

	return shader;
}

GLchar * getProgramInfoLog(GLuint program) {
	GLint length /* = 512 */;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
	GLchar *infoLog = malloc(sizeof(GLchar) * length);
	glGetProgramInfoLog(program, sizeof infoLog, NULL, infoLog);
	return infoLog;
}

struct mesh * create_mesh(int indexed) {
	GLuint buffers[2];
	struct mesh *mesh = malloc(sizeof(struct mesh));
	indexed = 1; // TODO remove and enforce
	return glGenBuffers(1 + indexed, buffers), (mesh->vbo = buffers[0]), (mesh->ibo = buffers[1]), mesh;
}

GLsizei calculate_stride(struct attrib *attributes) {
	GLsizei count = 0, i;
	for (i = 0; attributes[i].size != 0; i++) {
		struct attrib *attribute = &attributes[i];
		attribute->offset = count;
		count += sizeof(float)* attribute->size;
	}
	return count;
}

void destroy_mesh(struct mesh *mesh) {
	GLuint buffers[] = { mesh->vbo, mesh->ibo };
	glDeleteBuffers(1, buffers);
	free(mesh);
}