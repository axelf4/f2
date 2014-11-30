#include "GLDebugDrawer.h"
#include <iostream>

GLDebugDrawer::GLDebugDrawer() : debugMode(btIDebugDraw::DBG_DrawWireframe) {
	program = create_program(vertexShader, fragmentShader);
	glUseProgram(program);

	glGenBuffers(1, &vbo);
}

GLDebugDrawer::~GLDebugDrawer() {
	glDeleteProgram(program);
	glDeleteBuffers(1, &vbo);
}

void GLDebugDrawer::end(const float *modelViewProjection) {
	if (vertices.size() == 0) return;

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// float vertices[8] = { from.x(), from.y(), from.z(), 1, to.x(), to.y(), to.z(), 1 };
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_DYNAMIC_DRAW);

	glUseProgram(program);
	glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_FALSE, modelViewProjection);

	GLsizei stride = sizeof(float) * 3 * 2;

	GLint position = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(position);
	glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, stride, 0);
	GLint color = glGetAttribLocation(program, "color");
	glEnableVertexAttribArray(color);
	glVertexAttribPointer(color, 3, GL_FLOAT, GL_FALSE, stride, BUFFER_OFFSET(sizeof(float) * 3));

	glDrawArrays(GL_LINES, 0, vertices.size());

	glDisableVertexAttribArray(position);
	glDisableVertexAttribArray(color);

	vertices.clear();
}

void GLDebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color) {
	vertices.push_back(from.x());
	vertices.push_back(from.y());
	vertices.push_back(from.z());
	vertices.push_back(color.x());
	vertices.push_back(color.y());
	vertices.push_back(color.z());
	vertices.push_back(to.x());
	vertices.push_back(to.y());
	vertices.push_back(to.z());
	vertices.push_back(color.x());
	vertices.push_back(color.y());
	vertices.push_back(color.z());
}

void GLDebugDrawer::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) {}

void GLDebugDrawer::reportErrorWarning(const char* warningString) { std::cerr << warningString << std::endl; }

void GLDebugDrawer::draw3dText(const btVector3& location, const char* textString) {}

void GLDebugDrawer::setDebugMode(int debugMode) { this->debugMode = debugMode; }