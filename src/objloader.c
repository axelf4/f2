#include "objloader.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> // isspace
#include <assert.h>
#include <math.h>

#define SPACE " \t\n\v\f\r"
#define ATOI_CHARACTERS "+-0123456789"
#define ATOF_CHARACTERS ATOI_CHARACTERS "."

static int isNewLine(const char c) {
	return (c == '\r') || (c == '\n') || (c == '\0');
}

static float parseFloat(const char **token) {
	*token += strspn(*token, SPACE);
	float f = (float) atof(*token);
	*token += strspn(*token, ATOF_CHARACTERS);
	return f;
}

static unsigned int getIndex(const char **token, unsigned int size) {
	int idx = atoi(*token);
	*token += strspn(*token += strspn(*token, SPACE), ATOI_CHARACTERS);
	/*int idx = atoi(*token += strspn(*token, " \t"));
	*token += strcspn(*token, " \r\t/");*/
	return (idx > 0 ? idx - 1 : (idx < 0 ? /* negative value = relative */ size + idx : 0));
}

static char *parseText(const char **token) {
	unsigned int len = strspn(*token, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.");
	char *buffer = malloc(len * sizeof(char) + 1); // Allocate enough memory for text
	if (buffer == 0) return 0;
	memcpy(buffer, *token, len); // Copy string from line
	buffer[len] = '\0'; // Manually null-terminate string
	*token += len;
	return buffer;
}

static struct obj_group *create_group(char *name, unsigned int *numGroups, struct obj_group **groups) {
	struct obj_group *tmp = realloc(*groups, sizeof(struct obj_group) * (*numGroups + 1));
	if (!tmp) return 0;
	*groups = tmp;

	struct obj_group *group = *groups + *numGroups;
	group->name = name;
	group->materialIndex = 0;
	group->numFaces = 0;
	group->facesSize = 0;
	if ((group->faces = malloc(sizeof(unsigned int) * (group->facesCapacity = 3))) == 0) {
		free(name);
		return 0;
	}
	++*numGroups;
	return group;
}

struct obj_model *obj_load_model(const char *filename, const char *data, int flags) {
	// Array lists for the geometry, texture coordinate and normal storage
	unsigned int vertsSize = 0, uvsSize = 0, normsSize = 0, vertsCapacity = 2, uvsCapacity = 2, normsCapacity = 2;
	float *verts = 0, *uvs = 0, *norms = 0;
	// Model-wise booleans whether texture coordinates or normals exist
	unsigned int hasUVs = 0, hasNorms = 0;
	char *defaultGroupName = 0;
	unsigned int numGroups = 0;
	struct obj_group *groups = 0, *currentGroup;
	unsigned int numMaterials = 0;
	struct mtl_material **materials = 0;

	if(!(verts = malloc(sizeof(float) * vertsCapacity))) goto error;
	if(!(uvs = malloc(sizeof(float) * vertsCapacity))) goto error;
	if(!(norms = malloc(sizeof(float) * normsCapacity))) goto error;
	if(!(defaultGroupName = malloc(sizeof "default"))) goto error; // Allocate string on heap to be able to free it like the rest
	strcpy(defaultGroupName, "default");
	if(!(currentGroup = create_group(defaultGroupName, &numGroups, &groups))) goto error;

	const char *tok = data;
	while ('\0' != *tok) {
		if ('v' == *tok) {
			unsigned int *size, *capacity, hasZ = 1;
			float **list, x, y, z;
			if (' ' == *++tok) { // Position
				size = &vertsSize;
				capacity = &vertsCapacity;
				list = &verts;
			} else if ('t' == *tok) { // Texture coordinate
				size = &uvsSize;
				capacity = &uvsCapacity;
				list = &uvs;
				hasZ = 0;
			} else if ('n' == *tok) { // Normal
				size = &normsSize;
				capacity = &normsCapacity;
				list = &norms;
			}
			tok++; // Skip token (either ' ', 't' or 'n')
			if (*size + (hasZ ? 3 : 2) >= *capacity) {
				float *tmp = realloc(*list, sizeof(float) * (*capacity *= 2));
				if (!tmp) goto error;
				*list = tmp;
			}
			x = (*list)[(*size)++] = parseFloat(&tok); // X
			y = (*list)[(*size)++] = parseFloat(&tok); // Y
			if (hasZ) z = (*list)[(*size)++] = parseFloat(&tok); // Z
		} else if ('f' == *tok) {
			tok += 2; // Skip the characters 'f '
			unsigned int i, initialFacesSize, *facesSize = &currentGroup->facesSize, *facesCapacity = &currentGroup->facesCapacity, *numFaces = &currentGroup->numFaces, **faces = &currentGroup->faces;
			initialFacesSize = *facesSize;
			for (i = 0; !isNewLine(*tok); i++) {
				if (*facesSize + (1 + hasUVs + hasNorms) * (i >= 3 ? 3 : 1) >= *facesCapacity) {
					unsigned int *tmp = realloc(*faces, (*facesCapacity *= 2) * sizeof(unsigned int));
					if (!tmp) goto error;
					*faces = tmp;
				}

				if (i >= 3) {
					// More points than a triangle: triangulate it
					unsigned int lastFacesSize = *facesSize;

					(*faces)[(*facesSize)++] = (*faces)[initialFacesSize];
					if (hasUVs) (*faces)[(*facesSize)++] = (*faces)[initialFacesSize + 1];
					if (hasNorms) (*faces)[(*facesSize)++] = (*faces)[initialFacesSize + 1 + hasUVs];

					(*faces)[(*facesSize)++] = (*faces)[lastFacesSize - (1 + hasUVs + hasNorms)];
					if (hasUVs) (*faces)[(*facesSize)++] = (*faces)[lastFacesSize - (hasUVs + hasNorms)];
					if (hasNorms) (*faces)[(*facesSize)++] = (*faces)[lastFacesSize - (hasNorms)];

					currentGroup->numFaces += 2;
				}

				unsigned int x, y, z;
				x = (*faces)[(*facesSize)++] = getIndex(&tok, vertsSize); // Faces always define geometric data
				if (*tok++ == '/' /* && *tok != '/' */) {
					y = (*faces)[(*facesSize)++] = getIndex(&tok, uvsSize);
					hasUVs = 1;
				}
				if (*tok++ == '/') {
					z = (*faces)[(*facesSize)++] = getIndex(&tok, normsSize);
					hasNorms = 1;
				}
				currentGroup->numFaces++;
				tok += strspn(tok, " \t\r"); // Ignore spaces when checking for a newline
			}
			assert(i >= 2 && "Points and lines aren't supported.");
		} else if ('g' == *tok) {
			tok += 2; // Skip the 2 chars in "g "
			if (!(currentGroup = create_group(parseText(&tok), &numGroups, &groups))) goto error; // Create a new group
		} else if (strncmp("usemtl", tok, strlen("usemtl")) == 0) {
			tok += 7; // Skip the 7 chars in "usemtl "
			char *materialName = parseText(&tok); // Parse the name of the material
			if (!materialName) goto error;
			int existingGroup = 0;
			if (flags & OBJ_OPTIMIZE_MESHES) {
				// If a group using the same material is found: set currentGroup to it
				for (int i = 0; i < numGroups; i++) if (strcmp(materials[groups[i].materialIndex]->name, materialName) == 0) {
					currentGroup = groups + i;
					existingGroup = 1;
					break;
				}
			}
			if (!existingGroup) {
				// If the current group already has faces that doesn't use the new material we need to differentiate them
				if (currentGroup->numFaces > 0) {
					char *groupName = malloc(strlen(currentGroup->name) + 1); // Allocate memory for the name of the new group
					if (!groupName) {
						free(materialName);
						goto error;
					}
					strcpy(groupName, currentGroup->name); // Copy the name of the old group into the new one's
					// Create a new group sharing old one's name and link them
					if (!(currentGroup = create_group(groupName, &numGroups, &groups))) {
						free(materialName);
						free(groupName);
						goto error;
					}
				}
				for (unsigned int i = 0; i < numMaterials; i++) if (strcmp(materialName, materials[i]->name) == 0) {
					currentGroup->materialIndex = i;
					break;
				}
			}
			free(materialName);
		} else if (*tok == ' ' || *tok == '\n' || *tok == '\t' || *tok == 'r') {
			tok++; // Skip spaces or empty lines at beginning of line
			continue;
		} else if ('#' == *tok) {
			tok += strcspn(tok + 1, "\n") + 1; // Skip newlines or comments
		} else if (strncmp("mtllib", tok, strlen("mtllib")) == 0) {
			// TODO load multiple mtllibs as per definition: mtllib filename1 filename2 . . .
			// Use the .obj file's directory as a base
			tok += 7; // Skip the 7 chars in "mtllib "
			char *file = parseText(&tok);
			if (!file) goto error;
			unsigned int fileLen = strlen(file);
			unsigned int directoryLen = strrchr((char *) filename, '/') - filename + 1;
			if (directoryLen == 0) directoryLen = strrchr((char *) filename, '\\') - filename + 1;
			char *path = malloc(sizeof(char) * (directoryLen + fileLen + 1));
			if (!path) {
				free(file);
				goto error;
			}
			memcpy(path, filename, directoryLen);
			memcpy(path + directoryLen, file, fileLen);
			path[directoryLen + fileLen] = '\0';
			free(file);

			unsigned int numNewMaterials;
			struct mtl_material **newMaterials = load_mtl(path, &numNewMaterials);
			free(path);
			if (!newMaterials) goto error;

			struct mtl_material **tmp = realloc(materials, sizeof(struct mtl_material *) * (numMaterials + numNewMaterials));
			if (!tmp) {
				free(newMaterials);
				goto error;
			}
			materials = tmp;
			memcpy(materials + sizeof(struct mtl_material *) * numMaterials, newMaterials, sizeof(struct mtl_material *) * numNewMaterials);
			free(newMaterials);
			numMaterials += numNewMaterials;
		}
		tok = strchr(tok, '\n') + 1; // Skip to next line line
	}

	struct obj_model *model = malloc(sizeof(struct obj_model));
	if (!model) goto error;
	model->numGroups = numGroups;
	model->groups = groups;
	model->hasUVs = hasUVs;
	model->hasNorms = hasNorms;
	model->vertCount = vertsSize;
	model->uvCount = uvsSize;
	model->normCount = normsSize;
	model->verts = verts;
	model->uvs = uvs;
	model->norms = norms;
	model->numMaterials = numMaterials;
	model->materials = materials;
	return model;
error:
	free(verts);
	free(uvs);
	free(norms);
	free(defaultGroupName);
	if(groups) for (int i = 0; i < numGroups; i++) {
		free(groups[i].name);
		free(groups[i].faces);
	}
	free(groups);
	if (materials) for (int i = 0; i < numMaterials; i++) destroy_mtl_material(materials[i]);
	free(materials);
	return 0;
}

void obj_destroy_model(struct obj_model *model) {
	for (unsigned int i = 0; i < model->numGroups; i++) {
		struct obj_group group = model->groups[i];
		free(group.name);
		free(group.faces);
	}
	free(model->groups);
	for (unsigned int i = 0; i < model->numMaterials; i++) {
		struct mtl_material *material = model->materials[i];
		destroy_mtl_material(material);
	}
	if (model->materials != 0) free(model->materials);
	free(model->verts);
	free(model->uvs);
	free(model->norms);
	free(model);
}

struct mtl_material **load_mtl(const char *filename, unsigned int *numMaterials) {
	FILE *file = fopen(filename, "r");

	unsigned int size = 0, capacity = 2;
	struct mtl_material **materials = (struct mtl_material **) malloc(sizeof(struct mtl_material *) * capacity);

	struct mtl_material *cur; // Current material as specified by last newmtl

	char line[80];
	const char *tok;
	while (fgets(line, 80, file)) {
		tok += strspn(tok = line, " \t\r"); // Skip spaces at beginning of line
		if (strncmp("newmtl", tok, strlen("newmtl")) == 0) {
			if (size + 1 >= capacity) {
				struct mtl_material **tmp = (struct mtl_material **) realloc(materials, sizeof(struct mtl_material *) * (capacity *= 2));
				if (tmp == 0) {
					free(materials);
					perror("realloc failed inside load_mtl.");
					return 0;
				}
				materials = tmp;
			}
			cur = materials[size++] = (struct mtl_material *) malloc(sizeof(struct mtl_material));
			tok += 7; // Skip the 7 characters in "newmtl "
			cur->name = parseText(&tok);
			// TODO define defaults for rest
			cur->opacity = 1.f;
			cur->ambientTexture = 0;
		} else if (*tok == 'K' && (tok[1] == 'a' || tok[1] == 'd' || tok[1] == 's')) { // diffuse or specular
			float *color = tok[1] == 'a' ? cur->ambient : (tok[1] == 'd' ? cur->diffuse : cur->specular);
			tok += 3; // Skip the characters 'K' and 'a'/'d'/'s' and the space
			color[0] = parseFloat(&tok);
			color[1] = parseFloat(&tok);
			color[2] = parseFloat(&tok);
		} else if ((tok[0] == 'T' && tok[1] == 'r') || *tok == 'd') {
			tok += 2; // Skip the characters 'Tr' or 'd '
			cur->opacity = parseFloat(&tok);
		} else if (tok[0] == 'n' && tok[1] == 's') {
			tok += 3; // Skip the characters 'ns '
			cur->shininess = parseFloat(&tok);
		} else if (strncmp("map_Kd", tok, strlen("map_Kd")) == 0) {
			tok += 7; // Skip the 7 characters "map_Kd "
			cur->ambientTexture = parseText(&tok);
			tok += 0;
		}
	}
	fclose(file);

	struct mtl_material **retval = realloc(materials, sizeof(struct mtl_material *) * size);
	if (retval == 0) {
		free(materials);
		perror("realloc failed inside load_mtl.");
	}
	*numMaterials = size;
	return retval;
}

void destroy_mtl_material(struct mtl_material *material) {
	free(material->name);
	if (material->ambientTexture != 0) free(material->ambientTexture);
	free(material);
}

void obj_get_vertices(struct obj_model *model, struct obj_group *group, unsigned int *vertexCount, unsigned int *indexCount, float **vertices, unsigned int **indices) {
	int hasUVs = model->hasUVs, hasNorms = model->hasNorms;
	int flags = OBJ_INDICES;
	float *verts = model->verts, *uvs = model->uvs, *norms = model->norms;
	*vertices = malloc(sizeof(float) * (*vertexCount = group->numFaces * (3 + (hasUVs ? 2 : 0) + (hasNorms ? 3 : 0))));
	if (!(flags & OBJ_INDICES)) {
		*indexCount = 0;
		*indices = 0;
		for (unsigned int j = 0, vi = 0; j < group->facesSize;) {
			int vertIndex = group->faces[j++] * 3;
			*vertices[vi++] = verts[vertIndex++];
			*vertices[vi++] = verts[vertIndex++];
			*vertices[vi++] = verts[vertIndex];
			if (hasUVs) {
				int uvIndex = group->faces[j++] * 2;
				*vertices[vi++] = uvs[uvIndex++];
				*vertices[vi++] = uvs[uvIndex];
			}
			if (hasNorms) {
				int normIndex = group->faces[j++] * 3;
				*vertices[vi++] = norms[normIndex++];
				*vertices[vi++] = norms[normIndex++];
				*vertices[vi++] = norms[normIndex];
			}
		}
	} else {
		unsigned int vi = 0;
	   	*indices = malloc(sizeof(unsigned int) * (*indexCount = group->numFaces));
		for (unsigned int j = 0, k = 0; j < group->facesSize; k++) {
			unsigned int vertIndex = group->faces[j++] * 3, uvIndex = hasUVs ? group->faces[j++] * 2 : 0, normIndex = hasNorms ? group->faces[j++] * 3 : 0;

			int existing = 0;
			// Iterate through existing indices to find a match
			/* if (flags & OBJ_JOIN_IDENTICAL_VERTICES) for (unsigned int l = 0; l < j - (1 + hasUVs + hasNorms);) {
				if (vertIndex == group->faces[l++] * 3 && (!hasUVs || uvIndex == group->faces[l++] * 2) && (!hasNorms || normIndex == group->faces[l++] * 3)) {
					existing = 1;
					(*indices)[k] = (*indices)[(l - (1 + hasUVs + hasNorms)) / (1 + hasUVs + hasNorms)];
					break;
				}
			}*/
			if (!existing) {
				(*indices)[k] = vi / (3 + hasUVs * 2 + hasNorms * 3);
				(*vertices)[vi++] = verts[vertIndex++];
				(*vertices)[vi++] = verts[vertIndex++];
				(*vertices)[vi++] = verts[vertIndex];
				if (hasUVs) {
					(*vertices)[vi++] = uvs[uvIndex++];
					(*vertices)[vi++] = uvs[uvIndex];
				}
				if (hasNorms) {
					(*vertices)[vi++] = norms[normIndex++];
					(*vertices)[vi++] = norms[normIndex++];
					(*vertices)[vi++] = norms[normIndex];
				}
			}
		}
		*vertexCount = vi;
		float *tmp = realloc(*vertices, sizeof(float) * vi); // Reallocate the array to a save some memory
		if (tmp != 0) *vertices= tmp;
	}
}
