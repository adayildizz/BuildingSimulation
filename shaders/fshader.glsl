#version 410

layout(location = 0) out vec4 FragColor;

in vec4 baseColor; // This 'in' is unused in main, but kept for compatibility with existing code
in vec2 outTexCoord;      // Texture coordinates from vertex shader
in vec3 outWorldPos;      // World position from vertex shader
in vec3 outNormal_world;  // World-space normal from vertex shader
in vec4 outSplatWeights1234; // First 4 splat weights from vertex shader
in float outSplatWeight5;     // Fifth splat weight from vertex shader

// Texture samplers for terrain layers - now 5 textures
uniform sampler2D gTextureHeight0; // Sand
uniform sampler2D gTextureHeight1; // Grass
uniform sampler2D gTextureHeight2; // Dirt
uniform sampler2D gTextureHeight3; // Rock
uniform sampler2D gTextureHeight4; // Snow

// Separate texture sampler for objects
uniform sampler2D objectTexture;

// Height thresholds for blending textures (for terrain) - kept for compatibility but not used with splat weights
uniform float gHeight0;
uniform float gHeight1;
uniform float gHeight2;
uniform float gHeight3;
uniform float gHeight4; // New height threshold for snow

// Light Structure
struct DirectionalLight {
    vec3 color;
    float ambientIntensity;
    vec3 direction;          // Direction *from which* light comes (WORLD SPACE)
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

// NEW: Uniform to differentiate between terrain and object
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
                          tex4 * outSplatWeight5;          // Snow
                          
    return finalTexColor;
}

// Legacy function - kept for reference but not used with splat weights
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

void main()
{
    vec4 albedo;

    if (u_isTerrain) {
        // Use splat weights for terrain blending
        albedo = CalculateBlendedTextureColorFromWeights();
    } else {
        // Object rendering: Use the dedicated object texture sampler
        albedo = texture(objectTexture, outTexCoord);
    }

    vec3 N_world = normalize(outNormal_world);
    vec3 L_world = normalize(directionalLight.direction); // Points from surface TO light
    vec3 I_world = -L_world;                            // Incident light vector

    // Ambient Lighting
    vec3 ambient_lighting_color = directionalLight.ambientIntensity * directionalLight.color;

    // Diffuse Lighting
    float N_dot_L = max(dot(N_world, L_world), 0.0);
    vec3 diffuse_lighting_color = directionalLight.diffuseIntensity * N_dot_L * directionalLight.color;

    // Specular Lighting
    vec3 specular_lighting_color = vec3(0.0);
    if (N_dot_L > 0.0) // Only if light hits the front face
    {
        vec3 V_world = normalize(gViewPosition_world - outWorldPos); // View vector
        vec3 R_world = reflect(I_world, N_world);                    // Reflected light vector
        float V_dot_R = max(dot(V_world, R_world), 0.0);
        float specFactor = pow(V_dot_R, material.shininess);
        specular_lighting_color = material.specularIntensity * specFactor * directionalLight.color;
    }

    // Combine lighting components
    vec3 finalColor = albedo.rgb * (ambient_lighting_color + diffuse_lighting_color) + specular_lighting_color;

    FragColor = vec4(finalColor, albedo.a);
}