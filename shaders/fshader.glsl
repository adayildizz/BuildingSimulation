#version 410

layout(location = 0) out vec4 FragColor;

in vec4 outColor;
in vec2 outTexCoord;
in vec3 outWorldPos;

// Texture samplers - assuming up to 3 layers for now, matching main.cpp
uniform sampler2D gTextureHeight0;
uniform sampler2D gTextureHeight1;
uniform sampler2D gTextureHeight2;
uniform sampler2D gTextureHeight3;

// Height thresholds for blending - these are the upper transition points for each layer
uniform float gHeight0;
uniform float gHeight1;
uniform float gHeight2;
uniform float gHeight3;

// Function to calculate blended texture color based on height
vec4 CalculateBlendedTextureColor()
{
    vec4 blendedColor = vec4(0.0, 0.0, 0.0, 1.0); // Default to black if something goes wrong
    float currentHeight = outWorldPos.y;

    // Assumes gHeight values are ordered: gHeight0 < gHeight1 < gHeight2 ...
    // And that gHeightN is the height at which layer N is fully applied / layer N+1 starts blending in.

    if (currentHeight < gHeight0) {
        blendedColor = texture(gTextureHeight0, outTexCoord);
    } else if (currentHeight < gHeight1) {
        vec4 color0 = texture(gTextureHeight0, outTexCoord);
        vec4 color1 = texture(gTextureHeight1, outTexCoord);
        float factor = (currentHeight - gHeight0) / (gHeight1 - gHeight0);
        blendedColor = mix(color0, color1, factor);
    } else if (currentHeight < gHeight2) {
        vec4 color0 = texture(gTextureHeight1, outTexCoord);
        vec4 color1 = texture(gTextureHeight2, outTexCoord);
        float factor = (currentHeight - gHeight1) / (gHeight2 - gHeight1);
        blendedColor = mix(color0, color1, factor);
    } else if (currentHeight < gHeight3) {
        vec4 color1 = texture(gTextureHeight2, outTexCoord);
        vec4 color2 = texture(gTextureHeight3, outTexCoord);
        float factor = (currentHeight - gHeight2) / (gHeight3 - gHeight2);
        blendedColor = mix(color1, color2, factor);
    } else {
        blendedColor = texture(gTextureHeight3, outTexCoord);
    }
    return blendedColor;
}

void main()
{
    vec4 blendedTextureColor = CalculateBlendedTextureColor();
    FragColor = outColor * blendedTextureColor;
}