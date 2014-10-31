#version 400

attribute vec2 vertex;

uniform mat4 invProjection;
uniform mat4 trnModelView;

varying vec3 eyeDirection;

mat3 mat3_emu(mat4 m4) {
	return mat3(
		m4[0][0], m4[0][1], m4[0][2],
		m4[1][0], m4[1][1], m4[1][2],
		m4[2][0], m4[2][1], m4[2][2]);
}

void main() {
	vec4 v = vec4(vertex, 0.0, 1.0);

	vec3 unprojected = vec3(invProjection * v);
	eyeDirection = mat3_emu(trnModelView) * unprojected;

	gl_Position = v;
}