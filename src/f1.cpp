#include "f1.h"

#include <iostream>
#include <fstream>

float f1::opengl_version = -1;

float f1::getVersion() {
	if (opengl_version == -1) {
		const char *s = (const char*)glGetString(GL_VERSION);
		int dot = strchr(s, '.') - s;
		return opengl_version = s[dot - 1] - '0' + (float)(s[dot + 1] - '0') / 10;
	}
	else return opengl_version;
}

std::string f1::readFile(char *path) {
	std::ifstream in(path);
	std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	return contents;
}