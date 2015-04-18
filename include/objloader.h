/** A loader that takes a file name of an OBJ model and returns vertices and materials.
	@file objloader.h */

#ifndef OBJLOADER_H
#define OBJLOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#define OBJ_OPTIMIZE_MESHES

#define OBJ_FLAG_INDICES 0x1
#define OBJ_FLAG_OPTIMIZE_MESHES 0x2

	struct mtl_material {
		char *name;

		float ambient[3],
			diffuse[3],
			specular[3],
			emissive[3],
			reflection[3];

		float shininess;
		float opacity;

		// TODO add textures
		char *texPath;
	};

	struct obj_model_part {
		unsigned int vertexCount, indexCount, faceCount,
			materialIndex;
		float *vertices;
		unsigned int *indices;
	};

	/**
	  An OBJ model, consisting of a number of parts, given by #numParts and stored in #parts.
	  */
	struct obj_model {
		unsigned int numParts, /**< The number of parts of the type obj_model_part in #parts.*/
			numMaterials, /**< The number of materials. */
			hasUVs, /**< A boolean value of whether this model has texture coordinates. @warning This changes the memory layout. */
			hasNorms; /**< A boolean value of whether this model has normals. @warning This changes the memory layout. */
		struct obj_model_part **parts; /**< An array of pointers to parts of the type obj_model_part. */
		struct mtl_material **materials; /**< The complete set of materials. */
	};

	struct obj_model * load_obj_model(const char *filename, const char *data, int flags);

	void destroy_obj_model(struct obj_model *model);

	struct mtl_material **load_mtl(const char *filename, unsigned int *numMaterials);

	void destroy_mtl_material(struct mtl_material *material);

#ifdef __cplusplus
}
#endif

#endif