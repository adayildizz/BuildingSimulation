#version 410

layout(location = 0) out vec4 FragColor;

// Inputs from Vertex Shader
in vec4 baseColor;          // Vertex color (if used)
in vec2 outTexCoord;        // Texture coordinates
in vec3 outWorldPos;        // World position
in vec3 outNormal_world;    // World-space normal
in vec4 outFragPosLightSpace; // Fragment position from light's perspective

// Texture samplers
uniform sampler2D gTextureHeight0; // Terrain texture layer 0
uniform sampler2D gTextureHeight1; // Terrain texture layer 1
uniform sampler2D gTextureHeight2; // Terrain texture layer 2
uniform sampler2D gTextureHeight3; // Terrain texture layer 3
uniform sampler2D objectTexture;   // Texture for objects
uniform sampler2D shadowMap;       // Shadow map

// Uniforms for terrain blending
uniform float gHeight0;
uniform float gHeight1;
uniform float gHeight2;
uniform float gHeight3;

// Light structure
struct DirectionalLight {
    vec3 color;
    float ambientIntensity;
    vec3 direction;       // Direction *from which* light comes (WORLD SPACE)
    float diffuseIntensity;
};
uniform DirectionalLight directionalLight;

// Material properties
struct Material {
    float specularIntensity;
    float shininess;
};
uniform Material material;

// Global uniforms
uniform vec3 gViewPosition_world; // Camera's world position
uniform bool u_isTerrain;         // Flag to differentiate terrain and objects
uniform float u_shadowBias;      // Bias for shadow calculation

// Function to calculate blended texture color for terrain
vec4 CalculateBlendedTextureColor()
{
    vec4 finalTexColor;
    float currentHeight = outWorldPos.y;

    // Ensure blend ranges are valid to prevent division by zero or negative values
    float range0 = gHeight1 - gHeight0;
    float range1 = gHeight2 - gHeight1;
    float range2 = gHeight3 - gHeight2;

    if (currentHeight <= gHeight0) {
        finalTexColor = texture(gTextureHeight0, outTexCoord);
    } else if (currentHeight <= gHeight1) {
        vec4 tex0 = texture(gTextureHeight0, outTexCoord);
        vec4 tex1 = texture(gTextureHeight1, outTexCoord);
        float blendFactor = (range0 > 0.0001) ? (currentHeight - gHeight0) / range0 : (currentHeight > gHeight0 ? 1.0 : 0.0);
        finalTexColor = mix(tex0, tex1, clamp(blendFactor, 0.0, 1.0));
    } else if (currentHeight <= gHeight2) {
        vec4 tex1 = texture(gTextureHeight1, outTexCoord);
        vec4 tex2 = texture(gTextureHeight2, outTexCoord);
        float blendFactor = (range1 > 0.0001) ? (currentHeight - gHeight1) / range1 : (currentHeight > gHeight1 ? 1.0 : 0.0);
        finalTexColor = mix(tex1, tex2, clamp(blendFactor, 0.0, 1.0));
    } else if (currentHeight <= gHeight3) {
        vec4 tex2 = texture(gTextureHeight2, outTexCoord);
        vec4 tex3 = texture(gTextureHeight3, outTexCoord);
        float blendFactor = (range2 > 0.0001) ? (currentHeight - gHeight2) / range2 : (currentHeight > gHeight2 ? 1.0 : 0.0);
        finalTexColor = mix(tex2, tex3, clamp(blendFactor, 0.0, 1.0));
    } else {
        finalTexColor = texture(gTextureHeight3, outTexCoord);
    }
    return finalTexColor;
}

// Shadow Calculation Function
float CalculateShadowFactor()
{
    // Perspective divide for outFragPosLightSpace
    vec3 projCoords = outFragPosLightSpace.xyz / outFragPosLightSpace.w;

    // Transform to [0,1] range for texture coordinates
    projCoords = projCoords * 0.5 + 0.5;

    // Check if the fragment is outside the light's frustum's Z-depth range [0,1]
    if (projCoords.z > 1.0) { // If beyond light's far plane, consider it lit
        return 1.0;
    }
    // Check if the fragment is outside the light's frustum's XY-range [0,1]
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0; // Not in shadow map's XY area (e.g. due to GL_CLAMP_TO_BORDER not working perfectly), assume lit
    }
    
    // Get current fragment's depth from light's perspective
    float currentDepth = projCoords.z;
    
    // Sample shadow map
    float shadowMapDepth = texture(shadowMap, projCoords.xy).r; // .r because depth map is single channel

    // Check if fragment is in shadow
    float shadow = 1.0; // Assume lit
    if (currentDepth > shadowMapDepth + u_shadowBias) {
        shadow = 0.0; // In shadow
    }
    
    return shadow;
}

void main()
{
    vec4 albedo;
    if (u_isTerrain) {
        albedo = CalculateBlendedTextureColor();
    } else {
        albedo = texture(objectTexture, outTexCoord);
        // If objects have vertex colors you want to use (passed as 'baseColor'):
        // albedo *= baseColor; // Or however you intend to use vColor (baseColor)
    }

    // If albedo has 0 alpha from texture and you want it opaque for lighting
    if (albedo.a < 0.1) albedo.a = 1.0;

    vec3 N_world = normalize(outNormal_world);
    vec3 L_world = normalize(directionalLight.direction); // Points from surface TO light source
    vec3 I_world = -L_world;                              // Incident light vector (FROM light source TO surface)

    // Ambient Lighting
    vec3 ambient_lighting_color = directionalLight.ambientIntensity * directionalLight.color;

    // Diffuse Lighting
    float N_dot_L = max(dot(N_world, L_world), 0.0);
    vec3 diffuse_lighting_color = directionalLight.diffuseIntensity * N_dot_L * directionalLight.color;

    // Specular Lighting
    vec3 specular_lighting_color = vec3(0.0);
    if (N_dot_L > 0.0)
    {
        vec3 V_world = normalize(gViewPosition_world - outWorldPos); // View vector
        vec3 R_world = reflect(I_world, N_world);                    // Reflected light vector
        float V_dot_R = max(dot(V_world, R_world), 0.0);
        float specFactor = pow(V_dot_R, material.shininess);
        specular_lighting_color = material.specularIntensity * specFactor * directionalLight.color;
    }

    // Get shadow factor
    float shadowFactor = CalculateShadowFactor();

    // Combine lighting components, modulated by shadow factor
    // Shadow affects diffuse and specular, but not typically ambient (or less so)
    vec3 finalColor = albedo.rgb * ambient_lighting_color +
                      shadowFactor * (albedo.rgb * diffuse_lighting_color + specular_lighting_color);

    FragColor = vec4(finalColor, albedo.a);
}
