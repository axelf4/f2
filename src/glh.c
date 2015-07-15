#include "glh.h"
#include <stdlib.h>
#include <stdio.h>

GLsizei calculate_stride(struct attrib *attributes) {
	GLsizei count = 0, i;
	for (i = 0; attributes[i].size != 0; i++) {
		struct attrib *attribute = &attributes[i];
		attribute->offset = count;
		count += sizeof(float)* attribute->size;
	}
	return count;
}

const char *read_file(const char *filename) {
	char *buffer = 0;
	FILE *f = fopen(filename, "rb");
	if (f) {
		fseek(f, 0, SEEK_END);
		long length = ftell(f);
		fseek(f, 0, SEEK_SET);
		if ((buffer = malloc(length + 1)) == 0) return 0;
		buffer[length] = 0;
		if (buffer) fread(buffer, 1, length, f);
		fclose(f);
	}
	return buffer;
}

GLuint create_program(const char *vertexShader, const char *fragmentShader) {
	GLuint vert, frag, id = 0;
	if ((vert = compile_shader(GL_VERTEX_SHADER, vertexShader)) == 0) goto deleteVertex;
	if ((frag = compile_shader(GL_FRAGMENT_SHADER, fragmentShader)) == 0) goto deleteFragment;
	if ((id = glCreateProgram()) == 0) goto deleteFragment;
	glAttachShader(id, vert);
	glAttachShader(id, frag);

	glLinkProgram(id);
	GLint status;
	glGetProgramiv(id, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) goto deleteProgram;
	goto deleteFragment; // We don't need the shaders anymore
	
deleteProgram:
	glDeleteProgram(id);
	id = 0;
deleteFragment:
	glDeleteShader(frag);
deleteVertex:
	glDeleteShader(vert);
	return id;
}

GLuint compile_shader(GLenum type, const char *source) {
	GLuint shader = glCreateShader(type);
	if (shader == 0) {
		fprintf(stderr, "Error creating shader.\n");
		return 0;
	}
	const char **string = &source;
	glShaderSource(shader, 1, string, NULL);

	glCompileShader(shader);
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status); // Check shader compile status
	if (status == GL_FALSE) {
		GLint i;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &i);
		char *infoLog = malloc(sizeof(char) * i);
		glGetShaderInfoLog(shader, i, NULL, infoLog);
		const char *typeName = type == GL_VERTEX_SHADER ? "GL_VERTEX_SHADER" : (type == GL_FRAGMENT_SHADER ? "GL_FRAGMENT_SHADER" : "unknown");
		fprintf(stderr, "%s: %s\n", typeName, infoLog);
		free(infoLog);
		return 0;
	}

	return shader;
}

struct mesh * create_mesh(int indexed) {
	GLuint buffers[2];
	struct mesh *mesh = malloc(sizeof(struct mesh));
	if (mesh == 0) return 0;
	return glGenBuffers(1 + indexed, buffers), (mesh->vbo = buffers[0]), (mesh->ibo = buffers[1]), mesh;
}

void destroy_mesh(struct mesh *mesh) {
	GLuint buffers[] = { mesh->vbo, mesh->ibo };
	glDeleteBuffers(2, buffers);
	free(mesh);
}