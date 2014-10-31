#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

// #define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct {
		const char *name;

		float ambient[3];
		float diffuse[3];
		float specular[3];
		float transmittance[3];
		float emission[3];
		float shininess;
		float ior; // index of refraction
		float dissolve; // 1 == opaque; 0 == fully transparent
		int illum; // illumination model (see http://www.fileformat.info/format/material/)

		const char *ambient_texname;
		const char *diffuse_texname;
		const char *specular_texname;
		const char *normal_texname;
		// std::map<std::string, std::string> unknown_parameter;
	} material;

	typedef struct {

	} obj_model;

	static inline float parseFloat(char **token) {
		token += strspn(*token, " \t");
		float f = (float)atof(*token);
		token += strcspn(*token, " \t\r");
		return f;
	}

	static inline void parseFloat2(float *x, float *y, char** token) {
		*x = parseFloat(token);
		*y = parseFloat(token);
	}

	static inline void parseFloat3(float *x, float *y, float *z, char** token) {
		*x = parseFloat(token);
		*y = parseFloat(token);
		*z = parseFloat(token);
	}

	static inline int fixIndex(int idx, int size) {
		return (idx > 0 ? idx - 1 : (idx < 0 ? /* negative value = relative */ size + idx : 0));
	}

	typedef struct {
		LIST_HEAD(list);
		float f;
	} float_list;
	typedef struct {
		LIST_HEAD(list);
		int i;
	} int_list;

	inline obj_model * loadObj(char *filename) {
		FILE *fp = fopen(filename, "r");
		if (fp == NULL) {
			perror("Error opening file");
			return 0;
		}

		float_list v, vt, vn;
		int_list faces;

		char *t;
		if ((t = (char*)malloc(128 * sizeof(char))) == NULL) {
			printf("malloc failed\n");
			return 0;
		}
		// char line[128];
		// while (fgets(line, sizeof line, fp) != NULL) {
		while (fgets(t, 128, fp) != NULL) {
			if (*t == '\0') continue;

			if (t[0] == 'v') {
				// Vertex, texture coordinate or normal
				t += t[1] == ' ' ? 2 : 3; // Skip 'v ', 'vt ' or 'vn '
				vector *vector = (t[1] == ' ' ? &v : (t[1] == 'vt' ? &vt : &vn));
				float x, y, z;
				parseFloat3(&x, &y, &z, &t);
				vector_push(vector, &x);
				vector_push(vector, &y);
				vector_push(vector, &z);
			}
			else if (t[0] == 'f' && t[1] ==	' ') {
				// Face
				t += 2 + strspn(t, " \t"); // Skip 'f '

				while (!(*t == '\n' || *t == '\r' || *t == '\0')) {
					// Vertex index
					faces.push_back(fixIndex(atoi(t), v.size())); // vertex
					vector_push(&faces, &fixIndex(atoi(t), 0));
					t += strcspn(t, "/ \t\r");
					if (t[0] != '/') continue; // i, Solely vertex
					t++; // Skip '/'
					// Texcoord index i/j/k or i/j
					if (t[0] != '/') {
						faces.push_back(fixIndex(atoi(t), vt.size()));
						t += strcspn(t, "/ \t\r");
					}
					// Normal index i/j/k
					if (t[0] == '/') {
						t++;  // skip '/'
						faces.push_back(fixIndex(atoi(t), vn.size()));
						t += strcspn(t, "/ \t\r");
					}
					t += strspn(t, " \t\r");
				}
			}
		}
		free(t);

		vector_dispose(&v);
		vector_dispose(&vt);
		vector_dispose(&vn);
		vector_dispose(&faces);

		fclose(fp);
		return 0;
	}

#ifdef __cplusplus
}
#endif

#endif