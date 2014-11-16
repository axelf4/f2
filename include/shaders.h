#ifndef SHADERS_H
#define SHADERS_H

namespace shaders {
    // skybox.vert & skybox.frag
    const char *skyboxVertexSrc = GLSL(attribute vec2 vertex; uniform mat4 invProjection; uniform mat4 trnModelView; varying vec3 eyeDirection; void main() { eyeDirection = vec3(trnModelView * invProjection * (gl_Position = vec4(vertex, 0.0, 1.0))); /* trnModelView * unprojected */ });
    const char *skyboxFragmentSrc = GLSL(varying vec3 eyeDirection; uniform samplerCube texture; void main() { gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0)/*textureCube(texture, eyeDirection)*/; });

    // phong.vert
    const char phongVertexSrc[] = GLSL(
        attribute vec3 vertex;
    attribute vec2 texCoord;
    attribute vec3 normal;

    uniform mat4 m, v, p;
    uniform mat4 normalMatrix;

    varying vec4 position; // position of the vertex (and fragment) in world space
    varying vec3 N; // surface normal vector in world space
    varying vec2 f_texCoord;

    void main() {
        gl_Position = p * v * (position = m * vec4(vertex, 1.0)); // to clip coordinates

        position = vec4(vertex, 1.0);
        N = normalize(vec3(normalMatrix * vec4(normal, 0.0)));
        f_texCoord = texCoord;
    }
    );
    // phong.frag
    const char phongFragmentSrc[] = GLSL(
        varying vec4 position;
    varying vec3 N; // Normal in eye coord
    varying vec2 f_texCoord;

    uniform mat4 m, v, p;
    uniform vec4 eyeCoords;
    uniform mat4 vInv;
    uniform sampler2D tex;

    struct lightSource {
        vec4 position;
        vec4 diffuse;
        vec4 specular;
        float constantAttenuation, linearAttenuation, quadraticAttenuation;
        float spotCutoff, spotExponent;
        vec3 spotDirection;
    };
    lightSource light0 = lightSource(
        vec4(1.0, 0.0, 0.0, 0.0),
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
        vec4(1.0, 0.0, 0.0, 1.0),
        vec4(1.0, 1.0, 1.0, 1.0),
        15.0
        );

    void main() {
        vec3 normalDirection = normalize(N);
        vec3 viewDirection = normalize(vec3(vInv * vec4(0.0, 0.0, 0.0, 1.0) - position));
        vec3 lightDirection;
        float attenuation;

        // directional light?
        if (0.0 == light0.position.w) {
            attenuation = 1.0; // no attenuation
            lightDirection = normalize(vec3(light0.position));
        }
        else { // point light or spotlight (or other kind of light)
            // vec3 positionToLightSource = vec3(light0.position - position);
            vec3 positionToLightSource = vec3(eyeCoords - position);
            float distance = length(positionToLightSource);
            lightDirection = normalize(positionToLightSource);
            attenuation = 1.0 / (light0.constantAttenuation
                + light0.linearAttenuation * distance
                + light0.quadraticAttenuation * distance * distance);

            if (light0.spotCutoff <= 90.0) { // spotlight?
                float clampedCosine = max(0.0, dot(-lightDirection, light0.spotDirection));
                if (clampedCosine < cos(radians(light0.spotCutoff))) attenuation = 0.0; // outside of spotlight cone?
                else attenuation = attenuation * pow(clampedCosine, light0.spotExponent);
            }
        }

        vec3 ambientLighting = ambient * vec3(frontMaterial.ambient);

        vec3 diffuseReflection = attenuation
            * vec3(light0.diffuse) * vec3(frontMaterial.diffuse)
            * max(0.0, dot(normalDirection, lightDirection));

        vec3 specularReflection;
        // light source on the wrong side?
        if (dot(normalDirection, lightDirection) < 0.0) specularReflection = vec3(0.0, 0.0, 0.0); // no specular reflection
        else specularReflection = attenuation * vec3(light0.specular) * vec3(frontMaterial.specular)
            * pow(max(0.0, dot(reflect(-lightDirection, normalDirection), viewDirection)), frontMaterial.shininess); // light source on the right side

        // gl_FragColor = vec4(ambientLighting + diffuseReflection + specularReflection, 1.0);
        gl_FragColor = texture2D(tex, f_texCoord);
    }
    );
}

#endif
