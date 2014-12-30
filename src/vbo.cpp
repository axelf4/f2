#include "vbo.h"
#include "glh.h"
#include <iostream>

inline f1::vertex_data::~vertex_data() {}
inline f1::index_data::~index_data() {}

f1::vbo::vbo(attrib *attributes, int len, GLenum usage) : usage(usage), attributes(attributes), attribLen(len), stride(f1::calculateStride(attributes, len)) {
	// glGenVertexArrays(1, &vao); // Create a Vertex Array Object
	// glBindVertexArray(vao);
	glGenBuffers(1, &id); // Generate a Vertex Buffer Object
	// glBindBuffer(GL_ARRAY_BUFFER, id);

	// TODO attributes are not being changed by calculateStride so the offset never changes
	
	// glDeleteBuffers(1, &id); //delete it already, since the VAO still references it
	// glBindVertexArray(0);
}

f1::vbo::~vbo() {
	glDeleteBuffers(1, &id);
	// glDeleteVertexArrays(1, &vao);
	delete[] attributes;
}

void f1::vbo::bind() {
	glBindBuffer(GL_ARRAY_BUFFER, id); // Bind the Vertex Buffer Object
	// glBindVertexArray(vao);
}

void f1::vbo::bind(program *program) {
	glBindBuffer(GL_ARRAY_BUFFER, id);
	GLint location;
	for (int i = 0; i < attribLen; i++) {
		attrib attribute = attributes[i];
		if ((location = glGetAttribLocation(*program, attribute.name)) < 0) continue;
		glEnableVertexAttribArray(location);
		// std::cout << attribute.offset << " : " << (int) BUFFER_OFFSET(attribute.offset) << std::endl;
		glVertexAttribPointer(location, attribute.size, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(attribute.offset));
	}
}

void f1::vbo::unbind(program *program) {
	for (int i = 0; i < attribLen; i++) glDisableVertexAttribArray(glGetAttribLocation(*program, attributes[i].name));
	// glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void f1::vbo::setVertices(GLsizeiptr size, const GLvoid *data) {
	glBufferData(GL_ARRAY_BUFFER, size, data, usage);
}

f1::ibo::ibo(GLenum usage) : usage(usage) {
	glGenBuffers(1, &id);
}

f1::ibo::~ibo() {
	glDeleteBuffers(1, &id);
}

void f1::ibo::bind() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
}

void f1::ibo::unbind() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void f1::ibo::setIndices(GLsizeiptr size, const GLvoid *indices) {
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, usage);
}

int f1::calculateStride(attrib *attributes, int len) {
	int count = 0;
	for (int i = 0; i < len; i++) {
		attrib *attribute = &attributes[i];
		attribute->offset = count;
		count += sizeof(float)* attribute->size;
	}
	return count;
}

f1::mesh::mesh(bool indexed, attrib *attributes, int len, GLenum usage) {
	vertices = new vbo(attributes, len, usage);
	indices = indexed ? new ibo(usage) : NULL;
}

f1::mesh::~mesh() {
	delete vertices;
	if (indices != 0) delete indices;
}

void f1::mesh::bind(program *program) {
	vertices->bind(program);
	if (indices != NULL) indices->bind();
}

void f1::mesh::unbind(program *program) {
	vertices->unbind(program);
	if (indices != NULL) indices->unbind();
}

void f1::mesh::draw(GLenum mode, GLint first, GLsizei size) {
	if (indices == NULL) glDrawArrays(mode, first, size);
	else glDrawElements(mode, size, GL_UNSIGNED_INT, BUFFER_OFFSET(first));
}

f1::program::program(const GLchar *vertexShader, const GLchar *fragmentShader) {
	bool vertSuccess, fragSuccess;
	std::cout << "Vertex shader: " << std::endl;
	vert = compileShader(GL_VERTEX_SHADER, vertexShader, &vertSuccess);
	std::cout << "Fragment shader: " << std::endl;
	frag = compileShader(GL_FRAGMENT_SHADER, fragmentShader, &fragSuccess);
	if (!fragSuccess) glDeleteShader(vert);
	if (!vertSuccess || !fragSuccess) throw glsl_error("GLSL error");

	id = glCreateProgram();
	using namespace std;
	if (id == 0) throw "";
	glAttachShader(id, vert);
	glAttachShader(id, frag);

	glLinkProgram(id);
	int linked;
	glGetProgramiv(id, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE) {
		glDeleteProgram(id);
		glDeleteShader(vert);
		glDeleteShader(frag);
		throw glsl_error(getProgramInfoLog());
	}
}

f1::program::program(GLuint id) : id(id) {}

f1::program::~program() {
	glDeleteProgram(id);
	glDeleteShader(vert);
	glDeleteShader(frag);
}

GLuint f1::program::compileShader(GLenum type, const GLchar *src, bool *success) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, NULL);

	glCompileShader(shader);
	GLint i;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &i); // Check shader compile status
	if (i == GL_FALSE) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &i);
		GLchar *infoLog = new GLchar[i];
		glGetShaderInfoLog(shader, i, NULL, infoLog);
		std::cerr << infoLog << std::endl;
		delete[] infoLog;
		glDeleteShader(shader); /* We don't need the shader anymore. */
		*success = false;
	}

	*success = true;
	return shader;
}

GLchar * f1::program::getProgramInfoLog() {
	GLint len; // = 512
	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &len);
	GLchar *infoLog = new GLchar[len];
	glGetProgramInfoLog(id, sizeof(infoLog), 0, infoLog);
	return infoLog;
}

namespace f1 {
	std::ostream& operator<<(std::ostream& os, const f1::program& p) {
		GLsizei len;
		glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
		GLchar *infoLog = new GLchar[len];
		glGetProgramInfoLog(p, sizeof(infoLog), 0, infoLog);
		os << infoLog;
		delete[] infoLog;
		return os;
	}
}

GLint f1::program::getAttribLoc(const GLchar *name) {
	return glGetAttribLocation(id, name);
}
GLint f1::program::getUniformLoc(const GLchar *name) {
	return glGetUniformLocation(id, name);
}

GLuint f1::program::getId() const { return id; }
f1::program::operator GLuint() const { return id; }

void f1::program::operator()() { glUseProgram(id); }

f1::texture::texture() {
	glGenTextures(1, &id);
}

f1::texture::~texture() {
	glDeleteTextures(1, &id);
}

f1::texture::operator GLuint() { return id; }
