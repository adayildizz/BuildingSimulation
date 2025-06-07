#version 330 core

in vec2 fTexCoord;
in vec4 clipSpace;

uniform sampler2D dudvMap;
uniform sampler2D refractionTexMap;
uniform sampler2D reflectionTexMap;

uniform vec4 eyePosition;
uniform float uTime;

out vec4 fragColor;

void main() {
    vec2 ndc = (clipSpace.xy/clipSpace.w)*0.5 + 0.5;

    // animating distortion sampling 
    float moveFactor = mod(uTime*0.05, 1.0);
    vec2 distortionCoords = fTexCoord + vec2(moveFactor, moveFactor);
    vec2 distortion = texture(dudvMap, distortionCoords).rg*2.0-1.0;



    vec2 reflectionTexCoords = vec2(ndc.x, -ndc.y);
    vec2 refractionTexCoords = vec2(ndc.x, ndc.y);

    reflectionTexCoords += distortion*0.05;
    refractionTexCoords += distortion*0.05;

    reflectionTexCoords.x = clamp(reflectionTexCoords.x, 0.001, 0.999);
    reflectionTexCoords.y = clamp(reflectionTexCoords.y, -0.999, -0.001);
    refractionTexCoords = clamp(refractionTexCoords, 0.001, 0.999);

    vec4 reflectionColor = texture(reflectionTexMap, reflectionTexCoords);
    vec4 refractionColor = texture(refractionTexMap, refractionTexCoords);

    vec3 viewDir = normalize((clipSpace - eyePosition).xyz);
    float fresnelFactor = clamp(1.0 - dot(viewDir, vec3(0.0, 1.0, 0.0)), 0.0, 1.0);
    fresnelFactor = pow(fresnelFactor, 3.0);

    fragColor = refractionColor;
}
