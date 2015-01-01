#include "objloader.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static inline void skipSpaces(char **token) {
	// while(**token == ' ' || **token == '\t') (*token)++;
	*token += strspn(*token, " \t\r\0");
}

static inline float parseFloat(char **token) {
	float f = (float) atof(*token += strcspn(*token, "0123456789-."));
	*token += strcspn(*token, " \t\r/");
	return f;
}

static inline unsigned int getIndex(char **token, unsigned int size) {
	int idx = atoi(*token += strcspn(*token, "0123456789-"));
	*token += strcspn(*token, " \t\r/");
	return (idx > 0 ? idx - 1 : (idx < 0 ? /* negative value = relative */ size + idx : 0));
}

static inline char * parseText(char **token) {
	unsigned int len = strspn(*token, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.");
	char *retval = (char *) malloc(sizeof(char) * len + 1); // Allocate enough memory for text
	memcpy(retval, *token, len); // Copy string from line
	retval[len] = '\0'; // Manually null-terminate string
	*token += len;
	return retval;
}

struct group {
	char *name;
	unsigned int numFaces, facesSize, facesCapacity, *faces;
	struct mtl_material *material;
};

static inline struct group * create_group(char *name) {
	struct group *group = (struct group *) malloc(sizeof(struct group));
	group->name = name;
	group->numFaces = 0;
	group->facesSize = 0;
	group->faces = (unsigned int *) malloc(sizeof(unsigned int) * (group->facesCapacity = 2));
	group->material = 0;
	return group;	
}

static inline struct group * add_group(unsigned int *groupsSize, unsigned int *groupsCapacity, struct group ***groups, struct group *group) {
	if (*groupsSize + 1 >= *groupsCapacity) {
		*groups = (struct group **) realloc(*groups, sizeof(struct group *) * (*groupsCapacity *= 2));
	}
	return (*groups)[(*groupsSize)++] = group;
}

struct obj_model * load_obj_model(const char *filename) {
	unsigned int vertsSize = 0, vertsCapacity = 2;
	float *verts = (float *) malloc(sizeof(float) * vertsCapacity);
	unsigned int uvsSize = 0, uvsCapacity = 2;
	float *uvs = (float *) malloc(sizeof(float) * vertsCapacity);
	unsigned int normsSize = 0, normsCapacity = 2;
	float *norms = (float *) malloc(sizeof(float) * normsCapacity);

	unsigned int hasUVs = 0, hasNorms = 0;

	char *defaultGroupName = (char *) malloc(sizeof "default"); // Allocate string on heap instead of in data segment
	strcpy(defaultGroupName, "default");

	unsigned int groupsSize = 0, groupsCapacity = 2;
	struct group **groups = (struct group **) malloc(sizeof(struct group *) * groupsCapacity),
		*currentGroup = create_group(defaultGroupName);

	unsigned int materialsSize = 0;
	struct mtl_material **materials = 0;

	FILE *file = fopen(filename, "r");
	if (file == 0) {
		perror("Error reading file.");
		return 0;
	}
	char line[80], *tok;
	while(fgets(tok = line, 80, file)) {
		if ('\n' == *tok || '#' == *tok) continue;
		if ('v' == *tok) {
			unsigned int *size, *capacity, hasZ = 1;
			float **list;
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

			while(*size + (hasZ ? 3 : 2) >= *capacity) {
				*list = (float *) realloc(*list, sizeof(float) * (*capacity *= 2));
			}
			float x, y, z;
			x = (*list)[(*size)++] = parseFloat(&tok); // X
			y = (*list)[(*size)++] = parseFloat(&tok); // Y
			if (hasZ) z = (*list)[(*size)++] = parseFloat(&tok); // Z
		} else if ('f' == *tok) {
			unsigned int i, *facesSize = &currentGroup->facesSize, *facesCapacity = &currentGroup->facesCapacity, *numFaces = &currentGroup->numFaces,
				**faces = &currentGroup->faces;
			char *tmp;
			for(i = 0; *tok != '\0'; i++) {
				if (i % 3 == 0) tok = line;
				else if (i > 3 && (i - 1) % 3 == 0) tok = tmp;
				if ((i + 1) % 3 == 0) {
					tmp = tok;
					(*numFaces)++;
				}

				unsigned int x, y, z;
				x = getIndex(&tok, vertsSize); // Faces always has geometric data
				if (*(tok) == '/' && *++tok != '/') {
					y = getIndex(&tok, uvsSize);
					hasUVs = 1;
				}
				if (*(tok) == '/') {
					z = getIndex(&tok, normsSize);
					hasNorms = 1;
				}

				if (*facesSize + 1 + hasUVs + hasNorms >= *facesCapacity) {
					*faces = (unsigned int *) realloc(*faces, sizeof(unsigned int) * (*facesCapacity *= 2));
				}
				(*faces)[(*facesSize)++] = x;
				if (hasUVs) (*faces)[(*facesSize)++] = y;
				if (hasNorms) (*faces)[(*facesSize)++] = z;

				skipSpaces(&tok); // Ignore spaces when checking for a newline
			}
			assert(i >= 2 && "Points and lines aren't supported.");
		} else if ('g' == *tok) {
			if (currentGroup->numFaces > 0) add_group(&groupsSize, &groupsCapacity, &groups, currentGroup);
			tok += 2; // Skip the 2 chars in "g "
			currentGroup = create_group(parseText(&tok));
		} else if (strncmp("usemtl", tok, strlen("usemtl")) == 0) {
			// If the current group already has faces that doesn't use the new material we need to differentiate them
			if (currentGroup->numFaces > 0) {
				char *groupName = (char *) malloc(strlen(currentGroup->name) + 1);
				memcpy(groupName, currentGroup->name, strlen(currentGroup->name) + 1);
				add_group(&groupsSize, &groupsCapacity, &groups, currentGroup); // Push the current group
				currentGroup = create_group(groupName); // Create a new with the same name
			}
			tok += 7; // Skip the 7 chars in "usemtl "
			char *materialName = parseText(&tok);
			for (unsigned int i = 0; i < materialsSize; i++) {
				struct mtl_material *material = materials[i];
				if (strcmp(materialName, material->name) == 0) {
					currentGroup->material = material;
					break;
				}
			}
		} else if (strncmp("mtllib", tok, strlen("mtllib")) == 0) {
			// TODO make able to load multiple mtllibs as the definition: mtllib filename1 filename2 . . .
			// Use the .obj file's directory as a base
			tok += 7; // Skip the 7 chars in "mtllib "
			char *file = parseText(&tok);
			unsigned int fileLen = strlen(file);
			unsigned int directoryLen = strrchr((char *) filename, '/') - filename + 1;
			if (directoryLen == 0) directoryLen = strrchr((char *) filename, '\\') - filename + 1;
			char *path = (char *) malloc(sizeof(char) * (directoryLen + fileLen + 1));
			memcpy(path, filename, directoryLen);
			memcpy(path + directoryLen, file, fileLen);
			path[directoryLen + fileLen] = '\0';
			free(file);

			unsigned int numMaterials;
			struct mtl_material **newMaterials = load_mtl(path, &numMaterials);
			free(path);

			materials = (struct mtl_material **) realloc(materials, sizeof(struct mtl_material *) * (materialsSize + numMaterials));
			memcpy(materials + sizeof(struct mtl_material *) * materialsSize, newMaterials, sizeof(struct mtl_material *) * numMaterials);
			materialsSize += numMaterials;
		}
	}
	fclose(file);

	struct obj_model *model = (struct obj_model *) malloc(sizeof(struct obj_model));
	model->hasUVs = hasUVs;
	model->hasNorms = hasNorms;
	model->parts = (struct obj_model_part **) malloc(sizeof(struct obj_model_part *) * (model->numParts = groupsSize));
	model->numMaterials = materialsSize;
	model->materials = materials;
	for (unsigned int i = 0; i < groupsSize; i++) {
		struct group *group = groups[i];
		struct obj_model_part *part = model->parts[i] = (struct obj_model_part *) malloc(sizeof(struct obj_model_part));
		part->material = group->material;

		float *vertices = part->vertices = (float *) malloc(sizeof(float) * (part->vertexCount = group->numFaces * 3 * (3 + (hasUVs ? 2 : 0) + (hasNorms ? 3 : 0))));
		for (unsigned int i = 0, vi = 0; i < group->facesSize;) {
			int vertIndex = group->faces[i++] * 3;
			vertices[vi++] = verts[vertIndex++];
			vertices[vi++] = verts[vertIndex++];
			vertices[vi++] = verts[vertIndex];
			if (hasUVs) {
				int uvIndex = group->faces[i++] * 2;
				vertices[vi++] = uvs[uvIndex++];
				vertices[vi++] = uvs[uvIndex];
			}
			if (hasNorms) {
				int normIndex = group->faces[i++] * 3;
				vertices[vi++] = norms[normIndex++];
				vertices[vi++] = norms[normIndex++];
				vertices[vi++] = norms[normIndex];
			}
		}

		free(group->name);
		free(group->faces);
		free(group);
	}
	free(verts);
	free(uvs);
	free(norms);
	free(groups);
	return model;
}

void destroy_obj_model(struct obj_model *model) {
	for (unsigned int i = 0; i < model->numParts; i++) {
		struct obj_model_part *part = model->parts[i];
		free(part->vertices);
	}
	for (unsigned int i = 0; i < model->numMaterials; i++) {
		struct mtl_material *material = model->materials[i];
		destroy_mtl_material(material);
	}
	if (model->materials != 0) free(model->materials);
	free(model->parts);
	free(model);
}

struct mtl_material **load_mtl(const char *filename, unsigned int *numMaterials) {
	FILE *file = fopen(filename, "r");

	unsigned int size = 0, capacity = 2;
	struct mtl_material **materials = (struct mtl_material **) malloc(sizeof(struct mtl_material *) * capacity);

	struct mtl_material *cur; // Current material as specified by newmtl

	char line[80], *tok;
	while(fgets(tok = line, 80, file)) {
		skipSpaces(&tok);
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
			tok += 7; // Skip the 7 chars in "newmtl "
			cur->name = parseText(&tok);
			// TODO define defaults
			cur->opacity = 1.f;
			cur->texPath = 0;
		} else if (*tok == 'K' && (tok[1] == 'a' || tok[1] == 'd' || tok[1] == 's')) { // diffuse or specular
			float *color = tok[1] == 'a' ? cur->ambient : (tok[1] == 'd' ? cur->diffuse : cur->specular);
			color[0] = parseFloat(&tok);
			color[1] = parseFloat(&tok);
			color[2] = parseFloat(&tok);
		} else if ((tok[0] == 'T' && tok[1] == 'r') || *tok == 'd') cur->opacity = parseFloat(&tok);
		else if (tok[0] == 'n' && tok[1] == 's') cur->shininess = parseFloat(&tok);
		else if (strncmp("map_Kd", tok, strlen("map_Kd")) == 0) {
			// TODO Read texfile name
			tok += 7; // Skip the 7 chars in "map_Kd "
			cur->texPath = parseText(&tok);
		}
	}
	fclose(file);

	struct mtl_material **retval = (struct mtl_material **) realloc(materials, sizeof(struct mtl_material *) * size);
	if (retval == 0) {
		free(materials);
		perror("realloc failed inside load_mtl.");
	}
	*numMaterials = size;
	return retval;
}

void destroy_mtl_material(struct mtl_material *material) {
	free(material->name);
	if (material->texPath != 0) free(material->texPath);
	free(material);
}