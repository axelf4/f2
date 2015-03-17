#version 330

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 normal;

uniform mat4 m, v, p;

out vec2 TexCoord0;
out vec3 Normal0;
out vec3 WorldPos0;

void main() {
	mat4 mvp = p * v * m;
    gl_Position = mvp * vec4(vertex, 1.0);
    TexCoord0 = texCoord;
    Normal0 = (m * vec4(normal, 0.0)).xyz;
    WorldPos0 = (m * vec4(vertex, 1.0)).xyz;
}