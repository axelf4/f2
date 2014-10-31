// glh.h
#ifndef VBO_H
#define VBO_H

#include "f1.h"
#include <iostream>

namespace f1 {
	class texture {
		GLuint id;
	public:
		texture();
		~texture();
		// Returns the texture id
		operator GLuint();
	};

	struct attrib {
		const GLchar *name;
		// GLint location;
		/* The number of components per generic vertex attribute. Must be 1, 2, 3, 4. */
		GLint size;
		int offset;
	};
	static int calculateStride(attrib *attributes, int len);

	class glsl_error : public std::exception {
	private:
		const char *msg;
	public:
		glsl_error(const char *msg) : msg(msg) {};
		~glsl_error() throw() { delete[] msg; };
		const char *what() const throw() { return msg; };
	};

	// class shader {};

	class program;
	class program {
		GLuint id, vert, frag;

		static GLuint compileShader(GLenum type, const GLchar *source);
	public:
		program(const char *vertexShader, const char *fragmentShader);
		~program();

		GLint f1::program::getAttribLoc(const GLchar *name);
		GLint f1::program::getUniformLoc(const GLchar *name);

		/* Returns the program id. */
		GLuint getId() const;
		operator GLuint() const;

		/* Use the program. */
		void operator()();

		GLchar * getProgramInfoLog();
		/* Outputs the program info log. */
		friend std::ostream& operator<<(std::ostream& os, const program& p);
	};

	class vertex_data {
	public:
		virtual ~vertex_data();
		virtual void bind(program*) = 0;
		virtual void unbind(program*) = 0;
		virtual void setVertices(GLsizeiptr size, const GLvoid *vertices) = 0;
	};

	class vbo : public vertex_data {
		GLuint id, vao;
		GLenum usage;
		attrib *attributes;
		int attribLen;
		GLsizei stride;
	public:
		vbo(attrib attributes[], int len, GLenum usage = GL_STATIC_DRAW);
		~vbo();
		void bind();
		void bind(program *program);
		void unbind(program *program);
		void setVertices(GLsizeiptr size, const GLvoid *vertices);
		// void draw(GLenum mode, GLint first, GLsizei size);
	};

	class vertex_array : public vertex_data {};

	class index_data {
	public:
		virtual ~index_data();
		virtual void setIndices(GLsizeiptr size, const GLvoid *indices) = 0;
	};

	class ibo : public index_data {
		GLuint id;
		GLenum usage;
	public:
		ibo(GLenum usage);
		~ibo();
		void bind();
		void unbind();
		void setIndices(GLsizeiptr size, const GLvoid *indices);
	};

	class index_array : public index_data {};

	class mesh {
		vbo *vertices;
		ibo *indices;
	public:
		// usage = GL_STATIC_DRAW, GL_DYNAMIC_DRAW, ...
		mesh(bool indexed, attrib *attributes, int len, GLenum usage = GL_STATIC_DRAW);
		~mesh();
		void bind(program *program);
		void unbind(program *program);
		void draw(GLenum mode, GLint first, GLsizei size);
		inline vbo * getVertices() { return vertices; }
		inline ibo * getIndices() { return indices; }
	};
}

#endif