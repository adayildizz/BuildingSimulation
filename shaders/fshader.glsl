#version 410

layout(location = 0) out vec4 FragColor;

in vec2 outTexCoord;        // Texture coordinates from vertex shader
in vec3 outWorldPos;        // World position from vertex shader
in vec3 outNormal_world;    // World-space normal from vertex shader

// Texture samplers for terrain layers
uniform sampler2D gTextureHeight0;
uniform sampler2D gTextureHeight1;
uniform sampler2D gTextureHeight2;
uniform sampler2D gTextureHeight3;

// Height thresholds for blending textures
uniform float gHeight0;
uniform float gHeight1;
uniform float gHeight2;
uniform float gHeight3;

// Light Structure (must match uniform names used in C++ to set them)
struct DirectionalLight {
    vec3 color;             // Light color (e.g., white for sunlight)
    float ambientIntensity; // Intensity of ambient light component
    vec3 direction;         // Direction *from which* the light is coming (world space)
    float diffuseIntensity; // Intensity of diffuse light component
};

uniform DirectionalLight directionalLight; // Light properties

// (Optional for specular lighting)
// uniform vec3 gViewPosition_world; // Camera's world position

// Function to calculate blended texture color based on height (your existing logic, slightly refined for safety)
vec4 CalculateBlendedTextureColor()
{
    vec4 finalTexColor;
    float currentHeight = outWorldPos.y;

    // Blending logic:
    if (currentHeight <= gHeight0) {
        finalTexColor = texture(gTextureHeight0, outTexCoord);
    } else if (currentHeight <= gHeight1) {
        vec4 tex0 = texture(gTextureHeight0, outTexCoord);
        vec4 tex1 = texture(gTextureHeight1, outTexCoord);
        // Ensure denominator is not zero to prevent NaN/Inf
        float blendFactor = (gHeight1 - gHeight0 > 0.0001) ? (currentHeight - gHeight0) / (gHeight1 - gHeight0) : 0.0;
        finalTexColor = mix(tex0, tex1, clamp(blendFactor, 0.0, 1.0));
    } else if (currentHeight <= gHeight2) {
        vec4 tex1 = texture(gTextureHeight1, outTexCoord);
        vec4 tex2 = texture(gTextureHeight2, outTexCoord);
        float blendFactor = (gHeight2 - gHeight1 > 0.0001) ? (currentHeight - gHeight1) / (gHeight2 - gHeight1) : 0.0;
        finalTexColor = mix(tex1, tex2, clamp(blendFactor, 0.0, 1.0));
    } else if (currentHeight <= gHeight3) { // gHeight3 is the max height for the top-most blend
        vec4 tex2 = texture(gTextureHeight2, outTexCoord);
        vec4 tex3 = texture(gTextureHeight3, outTexCoord);
        float blendFactor = (gHeight3 - gHeight2 > 0.0001) ? (currentHeight - gHeight2) / (gHeight3 - gHeight2) : 0.0;
        finalTexColor = mix(tex2, tex3, clamp(blendFactor, 0.0, 1.0));
    } else { // Above gHeight3 (max terrain height), use the last texture
        finalTexColor = texture(gTextureHeight3, outTexCoord);
    }
    return finalTexColor;
}

void main()
{
    vec4 albedo = CalculateBlendedTextureColor(); // Base color from blended textures
    vec3 normal = normalize(outNormal_world);    // Ensure normal is unit length

    // Light direction should be normalized.
    vec3 lightDir = normalize(directionalLight.direction);
    vec3 toLightSourceDir = -lightDir; // Vector from surface point to light source

    // Ambient Light Component
    // The light's color (directionalLight.color) acts as a filter.
    vec3 ambient = directionalLight.ambientIntensity * albedo.rgb * directionalLight.color;

    // Diffuse Light Component
    // Depends on the angle between the normal and the direction to the light source.
    float diffFactor = max(dot(normal, toLightSourceDir), 0.0);
    vec3 diffuse = directionalLight.diffuseIntensity * diffFactor * albedo.rgb * directionalLight.color;

    // Combine ambient and diffuse components
    vec3 finalColor = ambient + diffuse;

    // The final color is the sum of lighting components.
    FragColor = vec4(finalColor, albedo.a); // Preserve alpha from the texture
}
