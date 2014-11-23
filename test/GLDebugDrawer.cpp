#include "GLDebugDrawer.h"
#include <iostream>

GLDebugDrawer::GLDebugDrawer() {
	program = create_program(vertexShader, fragmentShader);
	glUseProgram(program);
	GLint location = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(location);
	glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glGenBuffers(1, &vbo);
}

GLDebugDrawer::~GLDebugDrawer() {
	glDeleteProgram(program);
}

void GLDebugDrawer::setMatrices(float modelViewProjection[16]) {
	glUniformMatrix4fv(glGetUniformLocation(program, "mvp"), 1, GL_FALSE, modelViewProjection);
}

void GLDebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color) {
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	float vertices[8] = { from.x, from.y, from.z, 1, to.x, to.y, to.z, 1 };
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 8, vertices, GL_DYNAMIC_DRAW);

	glDrawArrays(GL_LINES, 0, 8);
}

void GLDebugDrawer::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) {}

void GLDebugDrawer::reportErrorWarning(const char* warningString) { std::cerr << warningString << std::endl; }

void GLDebugDrawer::draw3dText(const btVector3& location, const char* textString) {}

void GLDebugDrawer::setDebugMode(int debugMode) { this->debugMode = debugMode; }