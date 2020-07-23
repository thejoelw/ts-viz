#version 410 core

// #extension GL_ARB_shader_storage_buffer_object : require

#defines

// This is necessary so the fragment shader will not be called for occluded faces
// layout(early_fragment_tests) in;

#if EMPTY_FRAGMENT_SHADER

void main(void) {}

#else

uniform bool showTriangles;
uniform vec3 eyePos;

// layout(binding = NUM_FRAGMENTS_BINDING) uniform atomic_uint num_fragments;


in vec3 modelPosition;
flat in vec4 colorAmbient;
flat in vec4 colorDiffuse;
flat in vec4 colorSpecular;
flat in float shininess;
flat in uint renderModel;
layout(location = 0) out vec4 fragColor;

highp float rand(float seed) {
    highp float c = 43758.5453;
    highp float sn = mod(seed, 3.14);
    return fract(sin(sn) * c);
}


const bool blinn = true;
const vec4 lightColor = vec4(1.0, 1.0, 1.0, 1.0);
const float lightPower = 2000.0;
//const vec4 ambientColor = vec4(0.0, 0.0, 0.0, 1.0);
//const vec3 diffuseColor = vec3(188/256.0,143/256.0,143/256.0);
//const vec3 specColor = vec3(1.0, 1.0, 1.0);
//const float shininess = 32.0;
const float screenGamma = 2.2; // Assume the monitor is calibrated to the sRGB color space


void main(void) {
    if (showTriangles) {
        if (gl_FrontFacing) {
            fragColor = vec4(rand(gl_PrimitiveID), rand(gl_PrimitiveID + 0.3), rand(gl_PrimitiveID + 0.6), 0.0);
        } else {
            fragColor = vec4(1.0, 1.0, 1.0, 0.0);
        }
    } else {
        // TODO: This should come from the provoking vertex normal.
        vec3 xTangent = dFdx(modelPosition);
        vec3 yTangent = dFdy(modelPosition);
        vec3 faceNormal = normalize(cross(xTangent, yTangent));

        vec3 lightPos = eyePos + vec3(0.0, 0.0, 50.0);
//        vec3 lightPos = vec3(0.0, 10000.0, 0.0);
        vec3 lightDir = lightPos - modelPosition;
        float distance = length(lightDir);
        distance = distance * distance;
        lightDir = normalize(lightDir);

        float lambertian = max(dot(lightDir, faceNormal), 0.0);
        float specular = 0.0;

        if (lambertian > 0.0) {
          vec3 viewDir = normalize(eyePos - modelPosition);

          if (blinn) {
            vec3 halfDir = normalize(lightDir + viewDir);
            float specAngle = max(dot(halfDir, faceNormal), 0.0);
            specular = pow(specAngle, shininess);
          } else {
            vec3 reflectDir = reflect(-lightDir, faceNormal);
            float specAngle = max(dot(reflectDir, viewDir), 0.0);
            // note that the exponent is different here
            specular = pow(specAngle, shininess/4.0);
          }
        }
        vec4 colorLinear = colorAmbient * lightColor * lightPower / distance +
                           colorDiffuse * lambertian * lightColor * lightPower / distance +
                           colorSpecular * specular * lightColor * lightPower / distance;
        // apply gamma correction (assume ambientColor, diffuseColor and specColor
        // have been linearized, i.e. have no gamma correction in them)
        vec4 colorGammaCorrected = pow(colorLinear, vec4(1.0 / screenGamma));
        // use the gamma corrected color in the fragment
        fragColor = colorGammaCorrected;
    }
}

#endif
