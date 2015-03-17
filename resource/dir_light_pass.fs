#version 330

uniform sampler2D gPositionMap;
uniform sampler2D gColorMap;
uniform sampler2D gNormalMap;
uniform sampler2D depthMap;
uniform vec3 eyeCoords;
uniform float gMatSpecularIntensity;
uniform float gSpecularPower;
uniform int gLightType;
uniform vec2 gScreenSize;
uniform mat4 vInv;

out vec4 FragColor;

struct lightSource {
	vec4 position;
	vec4 diffuse;
	vec4 specular;
	float constantAttenuation, linearAttenuation, quadraticAttenuation;
	float spotCutoff, spotExponent;
	vec3 spotDirection;
};
lightSource light0 = lightSource(
	vec4(1.0, 1.0, 0.0, 0.0),
	vec4(1.0, 0.984, 0.816, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	1.0, 0.0, 0.0,
	180.0, 0.0,
	vec3(0.0, 0.0, 0.0)
	);
const vec3 ambient = vec3(0.2, 0.19, 0.18);

struct material {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};
material frontMaterial = material(
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	16.0);

void main() {
	vec2 texCoord = gl_FragCoord.xy / gScreenSize;
	
	vec3 position = texture(gPositionMap, texCoord).xyz;
	//vec3 eyeToScreen = vec3(texCoord.x * 800.0 / 600.0, texCoord.y, tan(90.0 / 2));
	//vec3 position = normalize(eyeToScreen) * texture(gDepthMap, texCoord).xyz;
	
	vec3 color = texture(gColorMap, texCoord).xyz;
	vec3 normal = normalize(texture(gNormalMap, texCoord).xyz);
	
	// vec3 lightDir = normalize(light0.position.xyz - position);
	vec3 lightDir = normalize(vec3(light0.position));
	float lambertian = max(dot(normal, lightDir), 0.0);
	
	vec3 viewDir = normalize(eyeCoords - position); // -position
	// calculate specular intensity (Blinn Phong)
	vec3 halfDir = normalize(lightDir + viewDir);
	float specAngle = max(dot(halfDir, normal), 0.0);
	float specular = pow(specAngle, 16.0); // frontMaterial.shininess
	
	vec3 ambientLighting = ambient * vec3(frontMaterial.ambient);
	vec3 diffuseLighting = lambertian * vec3(light0.diffuse) * vec3(frontMaterial.diffuse);
	vec3 specularLighting = specular * vec3(light0.specular) * vec3(frontMaterial.specular);
	
	FragColor = vec4(color * (ambientLighting + diffuseLighting + specularLighting), 1.0);
	gl_FragDepth = texture(depthMap, texCoord).r;
}