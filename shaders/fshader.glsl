#version 410

layout(location = 0) out vec4 FragColor;

in vec2 outTexCoord;      // Texture coordinates from vertex shader
in vec3 outWorldPos;      // World position from vertex shader
in vec3 outNormal_world;  // World-space normal from vertex shader

// Texture samplers for terrain layers (Unchanged)
uniform sampler2D gTextureHeight0;
uniform sampler2D gTextureHeight1;
uniform sampler2D gTextureHeight2;
uniform sampler2D gTextureHeight3;

// Height thresholds for blending textures (Unchanged)
uniform float gHeight0;
uniform float gHeight1;
uniform float gHeight2;
uniform float gHeight3;

// Light Structure (Existing)
struct DirectionalLight {
    vec3 color;             // Light color
    float ambientIntensity;   // Intensity of ambient light component
    vec3 direction;         // Direction *from which* the light is coming (WORLD SPACE)
    float diffuseIntensity;   // Intensity of diffuse light component
};
uniform DirectionalLight directionalLight;

// NEW: Material Properties for Specular Lighting
struct Material {
    float specularIntensity; // Strength of specular highlight
    float shininess;         // Shininess factor (exponent)
};
uniform Material material;

// NEW: Camera's world position (was optional/commented out, now needed for specular)
uniform vec3 gViewPosition_world; // Camera's world position

// Function to calculate blended texture color based on height (Unchanged)
vec4 CalculateBlendedTextureColor()
{
    vec4 finalTexColor;
    float currentHeight = outWorldPos.y;

    if (currentHeight <= gHeight0) {
        finalTexColor = texture(gTextureHeight0, outTexCoord);
    } else if (currentHeight <= gHeight1) {
        vec4 tex0 = texture(gTextureHeight0, outTexCoord);
        vec4 tex1 = texture(gTextureHeight1, outTexCoord);
        float blendFactor = (gHeight1 - gHeight0 > 0.0001) ? (currentHeight - gHeight0) / (gHeight1 - gHeight0) : 0.0;
        finalTexColor = mix(tex0, tex1, clamp(blendFactor, 0.0, 1.0));
    } else if (currentHeight <= gHeight2) {
        vec4 tex1 = texture(gTextureHeight1, outTexCoord);
        vec4 tex2 = texture(gTextureHeight2, outTexCoord);
        float blendFactor = (gHeight2 - gHeight1 > 0.0001) ? (currentHeight - gHeight1) / (gHeight2 - gHeight1) : 0.0;
        finalTexColor = mix(tex1, tex2, clamp(blendFactor, 0.0, 1.0));
    } else if (currentHeight <= gHeight3) {
        vec4 tex2 = texture(gTextureHeight2, outTexCoord);
        vec4 tex3 = texture(gTextureHeight3, outTexCoord);
        float blendFactor = (gHeight3 - gHeight2 > 0.0001) ? (currentHeight - gHeight2) / (gHeight3 - gHeight2) : 0.0;
        finalTexColor = mix(tex2, tex3, clamp(blendFactor, 0.0, 1.0));
    } else {
        finalTexColor = texture(gTextureHeight3, outTexCoord);
    }
    return finalTexColor;
}

void main()
{
    vec4 albedo = CalculateBlendedTextureColor(); // Base color from blended textures
    vec3 N_world = normalize(outNormal_world);   // Normalized surface normal in world space

    // --- Lighting Vectors (World Space) ---
    // directionalLight.direction is the direction *FROM WHICH* the light is coming.
    // L_world points from the surface TO the light source.
    vec3 L_world = normalize(directionalLight.direction);
    // I_world is the incident light vector (direction light travels TOWARDS the surface).
    vec3 I_world = -L_world;

    // --- Ambient Lighting ---
    vec3 ambient_lighting_color = directionalLight.ambientIntensity * directionalLight.color;

    // --- Diffuse Lighting ---
    float N_dot_L = max(dot(N_world, L_world), 0.0);
    vec3 diffuse_lighting_color = directionalLight.diffuseIntensity * N_dot_L * directionalLight.color;

    // --- Specular Lighting ---
    vec3 specular_lighting_color = vec3(0.0);
    if (N_dot_L > 0.0) // Only calculate specular if light hits the surface's front face
    {
        vec3 V_world = normalize(gViewPosition_world - outWorldPos); // View vector (surface to eye)
        vec3 R_world = reflect(I_world, N_world);                    // Reflected incident light vector
        
        float V_dot_R = max(dot(V_world, R_world), 0.0);
        float specFactor = pow(V_dot_R, material.shininess);
        
        specular_lighting_color = material.specularIntensity * specFactor * directionalLight.color;
    }

    // Combine lighting components
    // Ambient and diffuse are modulated by albedo. Specular is typically additive.
    vec3 finalColor = albedo.rgb * (ambient_lighting_color + diffuse_lighting_color) + specular_lighting_color;

    FragColor = vec4(finalColor, albedo.a); // Preserve alpha from the texture
    
}
