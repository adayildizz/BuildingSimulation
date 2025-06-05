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
    vec3 direction;       // As per your file: Direction *from which* light comes (WORLD SPACE)
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
uniform float u_shadowBias;      // Base bias for shadow calculation (still useful as a minimum)

// Function to calculate blended texture color for terrain
vec4 CalculateBlendedTextureColor()
{
    vec4 finalTexColor;
    float currentHeight = outWorldPos.y;

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
// Takes the world-space normal and the NdotL (diffuse factor before max(...,0.0)) for slope calculation.
// The NdotL passed here should be dot(N_world, LightToSurfaceDir_world) for the bias calculation.
// However, our existing L_world in main is `normalize(directionalLight.direction)` which is "direction light comes from",
// so for N.L in lighting, it effectively uses -L_world or adjusts interpretation.
// For slope scale bias, we need dot(Normal, LightDirection).
// Let's use the N_dot_L_lighting which is dot(N_world, L_world_for_lighting).
float CalculateShadowFactor(vec3 N_world_param, vec3 L_world_for_lighting_param)
{
    vec3 projCoords = outFragPosLightSpace.xyz / outFragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) { return 1.0; }
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }
    
    float currentDepth = projCoords.z;
    float shadowMapDepth = texture(shadowMap, projCoords.xy).r;

    // Calculate NdotL for slope bias
    float NdotL = max(dot(N_world_param, L_world_for_lighting_param), 0.0);

    // --- DEBUG STEP 3: Reduce the slope factor significantly ---
    float slope_factor = 0.002; // << TRY A MUCH SMALLER VALUE (was 0.02)
    float slope_dependent_bias = slope_factor * (1.0 - NdotL);
    
    float dynamicBias = u_shadowBias + slope_dependent_bias;

    // --- DEBUG STEP 4 (Optional): Output NdotL or dynamicBias ---
    // FragColor = vec4(NdotL, NdotL, NdotL, 1.0); return; // Visualize NdotL on terrain
    // FragColor = vec4(dynamicBias * 10.0, dynamicBias * 10.0, 0.0, 1.0); return; // Visualize dynamicBias

    float shadow = 1.0;
    if (currentDepth > shadowMapDepth + dynamicBias) {
        shadow = 0.0;
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
    }

    if (albedo.a < 0.1) albedo.a = 1.0; // Ensure opacity for lighting

    vec3 N_world = normalize(outNormal_world);

    // directionalLight.direction is the direction *FROM WHICH* the light comes.
    // For lighting calculations (N.L), L should be the vector from surface point *TO* the light.
    // So, L_for_lighting is -normalize(directionalLight.direction).
    // However, your existing code uses L_world = normalize(directionalLight.direction) and comments it as "Points from surface TO light".
    // Let's clarify and stick to one convention for L_world to be used in N.L and bias.
    // If directionalLight.direction is *FROM* light source (incident vector):
    // vec3 L_world_to_light = -normalize(directionalLight.direction); // Vector from surface point TO light source
    // If directionalLight.direction is *TOWARDS* light source (as if from origin to light pos):
    // vec3 L_world_to_light = normalize(directionalLight.direction); // Vector from surface point TO light source (assuming light is at infinity in this dir)

    // Based on your struct comment "Direction *from which* light comes" for directionalLight.direction:
    vec3 lightSourceDirection = normalize(directionalLight.direction); // This vector points FROM the light TO the surface (incident direction)
    vec3 L_world_for_lighting = -lightSourceDirection;                 // Vector from surface point TO light source

    // Ambient Lighting
    vec3 ambient_lighting_color = directionalLight.ambientIntensity * directionalLight.color;

    // Diffuse Lighting
    float N_dot_L_lighting = max(dot(N_world, L_world_for_lighting), 0.0);
    vec3 diffuse_lighting_color = directionalLight.diffuseIntensity * N_dot_L_lighting * directionalLight.color;

    // Specular Lighting
    vec3 specular_lighting_color = vec3(0.0);
    if (N_dot_L_lighting > 0.0)
    {
        vec3 V_world = normalize(gViewPosition_world - outWorldPos); // View vector
        // R = reflect(-L, N) where -L is incident vector. Here, lightSourceDirection is incident.
        vec3 R_world = reflect(lightSourceDirection, N_world); // Reflected light vector
        float V_dot_R = max(dot(V_world, R_world), 0.0);
        float specFactor = pow(V_dot_R, material.shininess);
        specular_lighting_color = material.specularIntensity * specFactor * directionalLight.color;
    }

    // Calculate shadow factor, passing necessary vectors for slope-scale bias
    float shadowFactor = CalculateShadowFactor(N_world, L_world_for_lighting);

    // Combine lighting components
    vec3 finalColor = albedo.rgb * ambient_lighting_color +
                      shadowFactor * (albedo.rgb * diffuse_lighting_color + specular_lighting_color);

    FragColor = vec4(finalColor, albedo.a);
}
