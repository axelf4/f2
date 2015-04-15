#ifndef DDSLOADER_H
#define DDSLOADER_H

#include <stddef.h>
#include <GL/glew.h>

#ifdef __cplusplus
extern "C" {
#endif

	extern GLuint dds_load_texture_from_memory(const char *data);

#ifdef __cplusplus
}
#endif

#endif