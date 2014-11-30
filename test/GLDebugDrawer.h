#include <LinearMath/btIDebugDraw.h>
#include "glh.h"
#include <vector>

// TODO rename to GLDebugDraw.h

class GLDebugDrawer : public btIDebugDraw {
	int debugMode;
	const char *vertexShader =
		"#version 110\n" \
		"attribute vec3 position;" \
		"attribute vec3 color;" \
		"uniform mat4 mvp;" \
		"varying vec3 vcolor;" \
		"void main() {" \
		"  gl_Position = mvp * vec4(position, 1.0f);" \
		"  vcolor = color;"
		"}";

	const char *fragmentShader =
		"#version 110\n" \
		"varying vec3 vcolor;" \
		"void main() {" \
		"  gl_FragColor = vec4(vcolor, 1.0);" \
		"}";
	GLuint program;
	GLuint vbo;
	std::vector<float> vertices;

public:
	GLDebugDrawer();
	virtual ~GLDebugDrawer();

	void end(const float *modelViewProjection);

	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color);

	virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color);

	virtual void reportErrorWarning(const char* warningString);

	virtual void draw3dText(const btVector3& location, const char* textString);

	virtual void setDebugMode(int debugMode);

	virtual int getDebugMode() const { return debugMode; }
};