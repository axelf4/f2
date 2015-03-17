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
	vec4(0.0, 0.0, 0.0, 1.0),
	vec4(0.8, 0.8, 0.8, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	1.0, 0.0, 0.0,
	180.0, 0.0,
	vec3(0.0, 0.0, 0.0)
	);
const vec3 ambient = vec3(0.0, 0.0, 0.0);

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
	15.0);

void main() {
	vec2 texCoord = gl_FragCoord.xy / gScreenSize;
	
	vec3 position = texture(gPositionMap, texCoord).xyz;
	//vec3 eyeToScreen = vec3(texCoord.x * 800.0 / 600.0, texCoord.y, tan(90.0 / 2));
	//vec3 position = normalize(eyeToScreen) * texture(gDepthMap, texCoord).xyz;
	
	vec3 color = texture(gColorMap, texCoord).xyz;
	vec3 normal = normalize(texture(gNormalMap, texCoord).xyz);
	
	vec3 viewDirection = normalize(vec3(vInv * vec4(0.0, 0.0, 0.0, 1.0) - position));
	vec3 lightDir;
	float attenuation;
	// directional light?
	if (0.0 == light0.position.w) {
		attenuation = 1.0; // no attenuation
		lightDir = normalize(vec3(light0.position));
	}
	else { // point light or spotlight (or other kind of light)
		// vec3 positionToLightSource = vec3(light0.position - position);
		vec3 positionToLightSource = eyeCoords - position;
		float distance = length(positionToLightSource);
		lightDir = normalize(positionToLightSource);
		attenuation = 1.0 / (light0.constantAttenuation
			+ light0.linearAttenuation * distance
			+ light0.quadraticAttenuation * distance * distance);
			if (light0.spotCutoff <= 90.0) { // spotlight?
			float clampedCosine = max(0.0, dot(-lightDir, light0.spotDirection));
			if (clampedCosine < cos(radians(light0.spotCutoff))) attenuation = 0.0; // outside of spotlight cone?
			else attenuation = attenuation * pow(clampedCosine, light0.spotExponent);
		}
	}
	vec3 ambientLighting = ambient * vec3(frontMaterial.ambient);
	vec3 diffuseReflection = attenuation
		* vec3(light0.diffuse) * vec3(frontMaterial.diffuse)
		* max(0.0, dot(normal, lightDir));
	vec3 specularReflection;
	// light source on the wrong side?
	if (dot(normal, lightDir) < 0.0) specularReflection = vec3(0.0, 0.0, 0.0); // no specular reflection
	else specularReflection = attenuation * vec3(light0.specular) * vec3(frontMaterial.specular)
		* pow(max(0.0, dot(reflect(-lightDir, normal), viewDirection)), frontMaterial.shininess); // light source on the right side
	FragColor = vec4(color * (ambientLighting + diffuseReflection + specularReflection), 1.0);
	gl_FragDepth = texture(depthMap, texCoord).r;
}