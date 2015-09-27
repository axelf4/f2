/** An OBJ model loader.
	Supports polygonal geometry and face elements.
	@file objloader.h */

#ifndef OBJLOADER_H
#define OBJLOADER_H

#ifdef __cplusplus
extern "C" {
#endif

	enum {
		/** Generates indices and populates \c obj_model_part.indices. */
		OBJ_INDICES = 0x1,
		/** Merge mesh parts that share the same material, effectively reducing the total number of meshes. */
		OBJ_OPTIMIZE_MESHES = 0x2,
		/** Use existing found indices, when ::OBJ_INDICES is specified. */
		OBJ_JOIN_IDENTICAL_VERTICES = 0x4
	};

	/** A MTL material definition. */
	struct mtl_material {
		char *name, /**< The name of the material. */
			*ambientTexture; /**< The path to the diffuse texture. */

		float ambient[3], /**< The ambient color, declared using *Ka r g b*, where the RGB values range between 0-1. */
			diffuse[3], /**< The diffuse color, declared using *Ka r g b*, where the RGB values range between 0-1. */
			specular[3], /**< The specular color, declared using *Ka r g b*, where the RGB values range between 0-1. */

			shininess, /**< The specular shininess, declared with *Ns #*, where # ranges between 0-1000. */
			opacity; /**< The transparency, declared with *Tr* or *d alpha*, ranges 0-1, where 1 is the default and not transparent at all. */
	};

	/** A part of a model, specifically a group, with at most one material. */
	struct obj_model_part {
		unsigned int vertexCount, /**< The number of vertices. */
			indexCount, /**< The number of indices. */
			faceCount, /**< The number of faces. */
			materialIndex; /**< The index of the material in the \c materials array. */
		float *vertices; /**< An array of vertices. */
		unsigned int *indices; /**< An array of indices. */
	};

	/** An OBJ model. */
	struct obj_model {
		unsigned int numParts, /**< The number of parts. */
			numMaterials, /**< The number of materials. */
			hasUVs, /**< A boolean value of whether this model has texture coordinates. @warning This changes the memory layout. */
			hasNorms; /**< A boolean value of whether this model has normals. @warning This changes the memory layout. */
		struct obj_model_part **parts; /**< An array of pointers to parts of type \c obj_model_part. */
		struct mtl_material **materials; /**< The set of materials. */
	};

	/** Loads an OBJ model.
		The filename must be passed to resolve the paths to the material definitions.
		@param filename The filename of the loaded model
		@param data The data of the model file, must be null-terminated
		@param flags Additional flags use when loading
		@return The loaded OBJ model */
	struct obj_model *load_obj_model(const char *filename, const char *data, int flags);

	/** Destroys an loaded OBJ model by freeing the used memory.
		@param model The model to free */
	void destroy_obj_model(struct obj_model *model);

	/** Loads a MTL material definition from a file.
		@param filename The path to the MTL file
		@param numMaterials A pointer to a integer which will be set to the number of materials
		@return An array of loaded materials */
	struct mtl_material **load_mtl(const char *filename, unsigned int *numMaterials);

	/** Destroys a loaded MTL material.
		@param material The material to destroy. */
	void destroy_mtl_material(struct mtl_material *material);

#ifdef __cplusplus
}
#endif

#endif
