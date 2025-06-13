#version 410

layout(location = 0) out vec4 FragColor;

in vec4 baseColor;
// World position from vertex shader// World-space normal from vertex shader
in vec4 outSplatWeights1234; // First 4 splat weights from vertex shader
in float outSplatWeight5;     // Fifth splat weight from vertex shader

in vec2 outTexCoord;        // Texture coordinates from vertex shader
in vec3 outWorldPos;        // World position from vertex shader
in vec3 outNormal_world;    // World-space normal from vertex shader
in vec4 outWorldPosLightSpace; // NEW: World position from the light's perspective

// Texture samplers for terrain layers - now 5 textures
uniform sampler2D gTextureHeight0; // Sand
uniform sampler2D gTextureHeight1; // Grass
uniform sampler2D gTextureHeight2; // Dirt
uniform sampler2D gTextureHeight3; // Rock
uniform sampler2D gTextureHeight4; // Snow

// Separate texture sampler for objects
uniform sampler2D objectTexture;

// Height thresholds for blending textures (for terrain) - kept for compatibility but not used with splat weights
// NEW: Shadow map sampler
uniform sampler2D shadowMap;
uniform bool u_shadowsEnabled;
//const bool u_shadowsEnabled = false;


// Height thresholds for blending textures (for terrain)
uniform float gHeight0;
uniform float gHeight1;
uniform float gHeight2;
uniform float gHeight3;
uniform float gHeight4; // New height threshold for snow

// Light Structure
struct DirectionalLight {
    vec3 color;
    float ambientIntensity;
    vec3 direction;         // Direction *from which* light comes (WORLD SPACE)
    float diffuseIntensity;
};
uniform DirectionalLight directionalLight;

// Material Properties
struct Material {
    float specularIntensity;
    float shininess;
};
uniform Material material;

// Camera's world position
uniform vec3 gViewPosition_world;

// Uniform to differentiate between terrain and object
uniform bool u_isTerrain;

// Function to calculate blended texture color using splat weights for 5 textures
vec4 CalculateBlendedTextureColorFromWeights()
{
    // Sample all terrain textures
    vec4 tex0 = texture(gTextureHeight0, outTexCoord); // Sand
    vec4 tex1 = texture(gTextureHeight1, outTexCoord); // Grass
    vec4 tex2 = texture(gTextureHeight2, outTexCoord); // Dirt
    vec4 tex3 = texture(gTextureHeight3, outTexCoord); // Rock
    vec4 tex4 = texture(gTextureHeight4, outTexCoord); // Snow
    
    // Blend using splat weights
    vec4 finalTexColor = tex0 * outSplatWeights1234.x +  // Sand
                         tex1 * outSplatWeights1234.y +  // Grass
                         tex2 * outSplatWeights1234.z +  // Dirt
                         tex3 * outSplatWeights1234.w +  // Rock
                         tex4 * outSplatWeight5;         // Snow
                         
    return finalTexColor;
}

// Legacy function - kept for reference but not used with splat weights
vec4 CalculateBlendedTextureColor()
{
    // ... (This function remains unchanged)
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

float CalculateShadowFactor()
{
    // Perform perspective divide
    vec3 projCoords = outWorldPosLightSpace.xyz / outWorldPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Get closest depth from light's POV (from shadow map)
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // Get current depth from light's POV
    float currentDepth = projCoords.z;

    // Check if the current fragment is in shadow
    float bias = max(0.005 * (1.0 - dot(normalize(outNormal_world), normalize(directionalLight.direction))), 0.0005);
    
    float shadow = (projCoords.z > 1.0) ? 0.0 : (currentDepth - bias > closestDepth ? 1.0 : 0.0);

    return shadow;
}

void main()
{
    vec4 albedo;

    if (u_isTerrain) {
        // Use splat weights for terrain blending
        albedo = CalculateBlendedTextureColorFromWeights();
    } else {
        albedo = texture(objectTexture, outTexCoord);
    }
    
    // By default, assume the fragment is fully lit (no shadow)
    float shadow = 1.0;
    
    // Only calculate shadows if the switch is enabled
    if (u_shadowsEnabled) {
        shadow = 1.0 - CalculateShadowFactor();
    }

    vec3 N_world = normalize(outNormal_world);
    vec3 L_world = normalize(directionalLight.direction);
    vec3 V_world = normalize(gViewPosition_world - outWorldPos);
    
    // Ambient Lighting
    vec3 ambient = directionalLight.ambientIntensity * directionalLight.color;

    // Diffuse Lighting
    float diff = max(dot(N_world, L_world), 0.0);
    vec3 diffuse = directionalLight.diffuseIntensity * diff * directionalLight.color;

    // Specular Lighting
    vec3 R_world = reflect(-L_world, N_world);
    float spec = pow(max(dot(V_world, R_world), 0.0), material.shininess);
    vec3 specular = material.specularIntensity * spec * directionalLight.color;

    // Combine lighting components
    // Apply the shadow factor to diffuse and specular light, but not ambient
    vec3 finalColor = (ambient + shadow * (diffuse + specular)) * albedo.rgb;

    FragColor = vec4(finalColor, albedo.a);
}
