#version 130
// phong.vert
in vec3 vertex;
in vec2 texCoord;
in vec3 normal;

uniform mat4 m, v, p;
uniform mat4 normalMatrix;

out vec4 position; // position of the vertex (and fragment) in world space
out vec3 N; // surface normal vector in world space
varying vec2 f_texCoord;

mat3 mat3_emu(mat4 m4) {
	return mat3(
		m4[0][0], m4[0][1], m4[0][2],
		m4[1][0], m4[1][1], m4[1][2],
		m4[2][0], m4[2][1], m4[2][2]);
}

void main() {
	gl_Position = p * v * (position = m * vec4(vertex, 1.0)); // to clip coordinates
	
	position = vec4(vertex, 1.0);
	N = normalize(mat3_emu(normalMatrix) * normal);
	f_texCoord = texCoord;
}