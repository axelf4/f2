/** A loader that takes a file name of an OBJ model and an array of it's data and returns vertices and materials.
	@file objloader.h */

#ifndef OBJLOADER_H
#define OBJLOADER_H

#ifdef __cplusplus
extern "C" {
#endif

	/** A flag that if set when loading a OBJ model will output both vertices and indices. */
#define OBJ_FLAG_INDICES 0x1
	/** A flag that if set when loading a OBJ model will reuse existing parts which share the same material, effectively reducing the number of meshes. */
#define OBJ_FLAG_OPTIMIZE_MESHES 0x2

	/** A structure representing an imported MTL material definition. */
	struct mtl_material {
		char *name, /**< The name of the material. */
			// TODO add textures
			*texPath; /**< The path to the diffuse texture. */

		float ambient[3], /**< The ambient color, declared using *Ka r g b*, where the RGB values range between 0-1. */
			diffuse[3], /**< The diffuse color, declared using *Ka r g b*, where the RGB values range between 0-1. */
			specular[3], /**< The specular color, declared using *Ka r g b*, where the RGB values range between 0-1. */

			shininess, /**< The specular shininess, declared with *Ns #*, where # ranges between 0-1000. */
			opacity; /**< The transparency, declared with *Tr* or *d alpha*, ranges 0-1, where 1 is the default and not transparent at all. */
	};

	/** A part of a model, specifically a group or an object, with at most one material. */
	struct obj_model_part {
		unsigned int vertexCount, /**< The number of vertices. */
			indexCount, /**< The number of indices. */
			faceCount, /**< The number of faces or triangles present. */
			materialIndex; /**< The index of this part's material. */
		float *vertices; /**< The vertices. */
		unsigned int *indices;/**< The indices. */
	};

	/** An OBJ model, consisting of a number of parts, given by #numParts and stored in #parts. */
	struct obj_model {
		unsigned int numParts, /**< The number of parts of the type obj_model_part in #parts.*/
			numMaterials, /**< The number of materials. */
			hasUVs, /**< A boolean value of whether this model has texture coordinates. @warning This changes the memory layout. */
			hasNorms; /**< A boolean value of whether this model has normals. @warning This changes the memory layout. */
		struct obj_model_part **parts; /**< An array of pointers to parts of the type obj_model_part. */
		struct mtl_material **materials; /**< The complete set of materials. */
	};

	/** Loads an OBJ model and returns it as a struct.
		The filename must be passed to resolve the paths to the material definitions.
		@param filename The filename of the loaded model
		@param data The data of the model file, must be null-terminated
		@param flags Additional flags use when loading
		@return The loaded OBJ model */
	struct obj_model * load_obj_model(const char *filename, const char *data, int flags);

	/** Destroys an loaded OBJ model by freeing the used memory.
		@param model The model to free */
	void destroy_obj_model(struct obj_model *model);

	/** Loads a MTL material definition from a file.
		@param filename The path to the MTL file
		@param numMaterials A pointer to a integer which will be set to the number of materials
		@return An array of loaded materials */
	struct mtl_material **load_mtl(const char *filename, unsigned int *numMaterials);

	/** Destroys the loaded MTL material.
		@param material The material to destroy. */
	void destroy_mtl_material(struct mtl_material *material);

#ifdef __cplusplus
}
#endif

#endif