varying vec3 eyeDirection;

uniform samplerCube tex;

void main() {
	gl_FragColor = textureCube(tex, eyeDirection);
	// gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}