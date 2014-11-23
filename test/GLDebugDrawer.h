#include <LinearMath/btIDebugDraw.h>
#include "glh.h"

// TODO rename to GLDebugDraw.h

class GLDebugDrawer : public btIDebugDraw {
	int debugMode;
	const char *vertexShader =
		"#version 110" \
		"attribute vec4 position;" \
		"uniform mat4 mvp;" \
		"void main() {" \
		"  gl_Position = mvp * position;" \
		"}";

	const char *fragmentShader =
		"#version 110" \
		"uniform vec4 color;" \
		"void main() {" \
		"  gl_FragColor = color;" \
		"}";
	GLuint program;
	GLuint vbo;

public:
	GLDebugDrawer();
	virtual ~GLDebugDrawer();

	void setMatrices(float modelViewProjection[16]);

	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color);

	virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color);

	virtual void reportErrorWarning(const char* warningString);

	virtual void draw3dText(const btVector3& location, const char* textString);

	virtual void setDebugMode(int debugMode);

	virtual int getDebugMode() const { return debugMode; }

};