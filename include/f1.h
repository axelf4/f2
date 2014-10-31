#ifndef F1_H
#define F1_H

#include <string>
#include <GL\glew.h>

#define FOR(i, x) for (unsigned int i = 0; i < x; i++)

// Converts degrees to radians.
#define degreesToRadians(angleDegrees) (angleDegrees * M_PI / 180.0)
// Converts radians to degrees.
#define radiansToDegrees(angleRadians) (angleRadians * 180.0 / M_PI)

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define CLAMP(x,a,b) MIN(MAX(x,a),b)

#define LOWER_CASE(p) for (; *p; ++p) *p = tolower(*p);

namespace f1 {
	extern float opengl_version;

	float getVersion();

	std::string readFile(char *file);
}

#endif