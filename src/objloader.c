#include "objloader.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> // isspace
#include <assert.h>

#define SPACE " \t\n\v\f\r"
#define ATOI_CHARACTERS "+-0123456789"
#define ATOF_CHARACTERS ATOI_CHARACTERS "."

static void skipSpacesBad(char **token) {
	// while(**token == ' ' || **token == '\t') (*token)++;
	*token += strspn(*token, " \r\t\n");
}

static int isNewLine(const char c) {
	return (c == '\r') || (c == '\n') || (c == '\0');
}

static float parseFloat(char **token) {
	float f = (float)atof(*token);
	*token += strspn(*token += strspn(*token, SPACE), ATOF_CHARACTERS);
	return f;
}

static unsigned int getIndex(char **token, unsigned int size) {
	int idx = atoi(*token);
	*token += strspn(*token += strspn(*token, SPACE), ATOI_CHARACTERS);
	/*int idx = atoi(*token += strspn(*token, " \t"));
	*token += strcspn(*token, " \r\t/");*/
	return (idx > 0 ? idx - 1 : (idx < 0 ? /* negative value = relative */ size + idx : 0));
}

static char * parseText(char **token) {
	unsigned int len = strspn(*token, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.");
	char *retval = (char *)malloc(sizeof(char) * len + 1); // Allocate enough memory for text
	memcpy(retval, *token, len); // Copy string from line
	retval[len] = '\0'; // Manually null-terminate string
	*token += len;
	return retval;
}

struct group {
	char *name;
	unsigned int numFaces, facesSize, facesCapacity, *faces;
	unsigned int materialIndex; // struct mtl_material *material;
	struct group *next; /**< A pointer to the next group in a linked list type structure. */
};

static struct group * create_group(char *name) {
	struct group *group = (struct group *) malloc(sizeof(struct group));
	group->name = name;
	group->numFaces = 0;
	group->facesSize = 0;
	group->faces = (unsigned int *)malloc(sizeof(unsigned int) * (group->facesCapacity = 3));
	group->materialIndex = 0;
	group->next = 0;
	return group;
}

struct obj_model * load_obj_model(const char *filename) {
	// Array lists for the geometry, texture coordinate and normal storage
	unsigned int vertsSize = 0, uvsSize = 0, normsSize = 0, vertsCapacity = 2, uvsCapacity = 2, normsCapacity = 2;
	float *verts = (float *)malloc(sizeof(float) * vertsCapacity), *uvs = (float *)malloc(sizeof(float) * vertsCapacity), *norms = (float *)malloc(sizeof(float) * normsCapacity);

	// Model-wise booleans whether texture coordinates or normals exist
	unsigned int hasUVs = 0, hasNorms = 0;

	char *defaultGroupName = (char *)malloc(sizeof "default"); // Allocate string on heap instead of in data segment to be able to free it like the rest
	strcpy(defaultGroupName, "default");

	unsigned int groupsSize = 1; // TODO rename to numGroups // Store the number of groups to not have to traverse them to see the length
	struct group *currentGroup = create_group(defaultGroupName), *firstGroup = currentGroup, *lastGroup = currentGroup;

	unsigned int materialsSize = 0;
	struct mtl_material **materials = 0;

	FILE *file = fopen(filename, "r");
	if (file == 0) {
		perror("Error reading file.");
		return 0;
	}
	char line[80], *tok;
	while (fgets(tok = line, 80, file)) {
		tok += strspn(tok, " \t\r"); // Skip spaces at beginning of line
		if ('\n' == *tok || '#' == *tok) continue;
		if ('v' == *tok) {
			unsigned int *size, *capacity, hasZ = 1;
			float **list, x, y, z;
			if (' ' == *++tok) { // Position
				size = &vertsSize;
				capacity = &vertsCapacity;
				list = &verts;
			}
			else if ('t' == *tok) { // Texture coordinate
				size = &uvsSize;
				capacity = &uvsCapacity;
				list = &uvs;
				hasZ = 0;
			}
			else if ('n' == *tok) { // Normal
				size = &normsSize;
				capacity = &normsCapacity;
				list = &norms;
			}
			tok++; // Skip token (spaces are no problem; the letters 't'/'v' are)
			while (*size + (hasZ ? 3 : 2) >= *capacity) {
				*list = (float *)realloc(*list, sizeof(float) * (*capacity *= 2));
			}
			x = (*list)[(*size)++] = parseFloat(&tok); // X
			y = (*list)[(*size)++] = parseFloat(&tok); // Y
			if (hasZ) z = (*list)[(*size)++] = parseFloat(&tok); // Z
		}
		else if ('f' == *tok) {
			tok += 2; // Skip the characters 'f '
			unsigned int i, initialFacesSize, *facesSize = &currentGroup->facesSize, *facesCapacity = &currentGroup->facesCapacity, *numFaces = &currentGroup->numFaces, **faces = &currentGroup->faces;
			initialFacesSize = *facesSize;
			for (i = 0; !isNewLine(*tok); i++) {
				if (*facesSize + (1 + hasUVs + hasNorms) * (i >= 3 ? 3 : 1) >= *facesCapacity) {
					*faces = (unsigned int *)realloc(*faces, sizeof(unsigned int) * (*facesCapacity *= 2));
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
				if (*tok == '/' && *++tok != '/') {
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
		}
		else if ('g' == *tok) {
			tok += 2; // Skip the 2 chars in "g "
			struct group *oldgroup = lastGroup;
			oldgroup->next = (currentGroup = lastGroup = create_group(parseText(&tok))); // Create a new group and leave a reference in the linked list structure
			groupsSize++;
		}
		else if (strncmp("usemtl", tok, strlen("usemtl")) == 0) {
			tok += 7; // Skip the 7 chars in "usemtl "
			char *materialName = parseText(&tok); // Parse the name of the material

#ifdef OBJ_OPTIMIZE_MESHES
			// If a group using the same material is found: set currentGroup to it
			int existingGroup = 0;
			struct group *group = firstGroup;
			do {
				if (strcmp(materials[group->materialIndex]->name, materialName) == 0) {
					currentGroup = group;
					existingGroup = 1;
					break;
				}
			} while ((group = group->next) != 0);
			if (existingGroup) continue;
#endif

			// If the current group already has faces that doesn't use the new material we need to differentiate them
			if (currentGroup->numFaces > 0) {
				char *groupName = (char *)malloc(strlen(currentGroup->name) + 1); // Allocate memory for the name of the new group
				memcpy(groupName, currentGroup->name, strlen(currentGroup->name) + 1); // Copy the name of the old group into the new one's
				struct group *oldgroup = lastGroup;
				oldgroup->next = (currentGroup = lastGroup = create_group(groupName)); // Create a new group sharing the name of the old and link them
				groupsSize++;
			}
			for (unsigned int i = 0; i < materialsSize; i++) {
				struct mtl_material *material = materials[i];
				if (strcmp(materialName, material->name) == 0) {
					currentGroup->materialIndex = i; // currentGroup->material = material;
					break;
				}
			}
		}
		else if (strncmp("mtllib", tok, strlen("mtllib")) == 0) {
			// TODO make able to load multiple mtllibs as the definition: mtllib filename1 filename2 . . .
			// Use the .obj file's directory as a base
			tok += 7; // Skip the 7 chars in "mtllib "
			char *file = parseText(&tok);
			unsigned int fileLen = strlen(file);
			unsigned int directoryLen = strrchr((char *)filename, '/') - filename + 1;
			if (directoryLen == 0) directoryLen = strrchr((char *)filename, '\\') - filename + 1;
			char *path = (char *)malloc(sizeof(char) * (directoryLen + fileLen + 1));
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
	model->parts = (struct obj_model_part **) malloc(sizeof(struct obj_model_part *) * groupsSize);
	model->numMaterials = materialsSize;
	model->materials = materials;
	unsigned int i = 0;
	struct group *group = firstGroup, *nextGroup;
	unsigned int complete = 0;
	do {
		nextGroup = group->next; // Need this reference before freeing the group
		// Skip the group if empty, don't care about the empty space in the parts array
		if (group->facesSize == 0) continue;
		struct obj_model_part *part = model->parts[i] = (struct obj_model_part *) malloc(sizeof(struct obj_model_part));
		part->materialIndex = group->materialIndex; // part->material = group->material;
		// part->faceCount = group->facesSize / (1 + hasUVs + hasNorms); // group->numFaces
		part->faceCount = group->numFaces;

#ifdef OBJ_VERTICES_ONLY
		float *vertices = part->vertices = (float *)malloc(sizeof(float) * (part->vertexCount = group->numFaces * 3 * (3 + (hasUVs ? 2 : 0) + (hasNorms ? 3 : 0))));
		part->indices = 0;
		for (unsigned int j = 0, vi = 0; j < group->facesSize;) {
			int vertIndex = group->faces[j++] * 3;
			vertices[vi++] = verts[vertIndex++];
			vertices[vi++] = verts[vertIndex++];
			vertices[vi++] = verts[vertIndex];
			if (hasUVs) {
				int uvIndex = group->faces[j++] * 2;
				vertices[vi++] = uvs[uvIndex++];
				vertices[vi++] = uvs[uvIndex];
			}
			if (hasNorms) {
				int normIndex = group->faces[j++] * 3;
				vertices[vi++] = norms[normIndex++];
				vertices[vi++] = norms[normIndex++];
				vertices[vi++] = norms[normIndex];
			}
		}
#else
		unsigned int verticesSize = 0, verticesCapacity = 2;
		float *vertices = (float *)malloc(sizeof(float) * vertsCapacity);
		unsigned int *indices = part->indices = (unsigned int *)malloc(sizeof(unsigned int) * (part->indexCount = group->numFaces)),
			vi = 0;
		for (unsigned int j = 0, k = 0; j < group->facesSize; k++) {
			int vertIndex = group->faces[j++] * 3,
				uvIndex = hasUVs ? group->faces[j++] * 2 : 0,
				normIndex = hasNorms ? group->faces[j++] * 3 : 0;

			int existing = 0;
			for (unsigned int l = 0; l < j;) {
				if (vertIndex == group->faces[l++] &&
					(!hasUVs || uvIndex == group->faces[l++]) &&
					(!hasNorms || normIndex == group->faces[l++])) {
					if (l == j) break; // Compared against itself
					existing = 1;
					indices[k] = l;
					break;
				}
			}
			if (!existing) {
				indices[k] = vi / (3 + hasUVs * 2 + hasNorms * 3);
				if (vi + (3 + hasUVs * 2 + hasNorms * 3) >= verticesCapacity) {
					vertices = (float *)realloc(vertices, sizeof(float) * (verticesCapacity *= 4));
				}
				vertices[vi++] = verts[vertIndex++];
				vertices[vi++] = verts[vertIndex++];
				vertices[vi++] = verts[vertIndex];
				if (hasUVs) {
					vertices[vi++] = uvs[uvIndex++];
					vertices[vi++] = uvs[uvIndex];
				}
				if (hasNorms) {
					vertices[vi++] = norms[normIndex++];
					vertices[vi++] = norms[normIndex++];
					vertices[vi++] = norms[normIndex];
				}
			}
		}
		complete += vi;
		part->vertexCount = vi;
		part->vertices = vertices;
#endif

		free(group->name);
		free(group->faces);
		free(group);
		i++; // Increment for next group
	} while ((group = nextGroup) != 0);
	printf("Complete %d\n.", complete);
	model->numParts = i; // Set the model's number of parts to the actual number of groups/parts as counted
	free(verts);
	free(uvs);
	free(norms);
	return model;
}

void destroy_obj_model(struct obj_model *model) {
	for (unsigned int i = 0; i < model->numParts; i++) {
		struct obj_model_part *part = model->parts[i];
		free(part->vertices);
		free(part->indices);
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

	struct mtl_material *cur; // Current material as specified by last newmtl

	char line[80], *tok;
	while (fgets(tok = line, 80, file)) {
		tok += strspn(tok, " \t\r"); // Skip spaces at beginning of line
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
			cur->texPath = 0;
		}
		else if (*tok == 'K' && (tok[1] == 'a' || tok[1] == 'd' || tok[1] == 's')) { // diffuse or specular
			float *color = tok[1] == 'a' ? cur->ambient : (tok[1] == 'd' ? cur->diffuse : cur->specular);
			tok += 3; // Skip the characters 'K' and 'a'/'d/'s' and the space
			color[0] = parseFloat(&tok);
			color[1] = parseFloat(&tok);
			color[2] = parseFloat(&tok);
		}
		else if ((tok[0] == 'T' && tok[1] == 'r') || *tok == 'd') {
			tok += 2; // Skip the characters 'Tr' or 'd '
			cur->opacity = parseFloat(&tok);
		}
		else if (tok[0] == 'n' && tok[1] == 's') {
			tok += 3; // Skip the characters 'ns '
			cur->shininess = parseFloat(&tok);
		}
		else if (strncmp("map_Kd", tok, strlen("map_Kd")) == 0) {
			tok += 7; // Skip the 7 characters "map_Kd "
			cur->texPath = parseText(&tok);
			tok += 0;
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