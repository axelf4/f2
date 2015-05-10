/** This file contains the image codec operations for DDS files.
	@file bitmap_dds.h */

#ifndef DDSLOADER_H
#define DDSLOADER_H

#include <stddef.h>
#include <GL/glew.h>

#ifdef __cplusplus
extern "C" {
#endif

	enum {
		DDS_FLIP_UVS = 1 /**< Flips all UV coordinates along the y-axis. */
	};

	extern GLuint dds_load_texture_from_memory(const char *data, int flags);

#ifdef __cplusplus
}
#endif

#endif