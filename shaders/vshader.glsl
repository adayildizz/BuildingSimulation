#version 410

layout (location = 0) in vec4 vPosition;   // Vertex position (model space)
layout (location = 1) in vec2 vTexCoord;
layout (location = 2) in vec3 vNormal;     // Vertex normal (model space)
layout (location = 3) in vec4 vColor;

uniform mat4 gVP;          // Combined View * Projection matrix
uniform mat4 gModelMatrix; // Model matrix (transforms model to world space)
uniform mat4 gLightSpaceMatrix; // NEW: Transforms world to light space

out vec4 baseColor;
out vec2 outTexCoord;        // Pass texture coordinates to fragment shader
out vec3 outWorldPos;        // Pass world position to fragment shader
out vec3 outNormal_world;    // Pass normal (in world space) to fragment shader
out vec4 outWorldPosLightSpace; // NEW: Pass light-space position to fragment shader

void main()
{
    // Transform vertex position to world space
    vec4 worldPos_vec4 = gModelMatrix * vPosition; // Use vPosition directly
    outWorldPos = worldPos_vec4.xyz;

    // Transform vertex position to clip space (for the camera)
    gl_Position = gVP * worldPos_vec4;
    
    // Transform normal to world space
    outNormal_world = normalize(mat3(transpose(inverse(gModelMatrix))) * vNormal);
    
    // Pass other data
    baseColor = vColor;
    outTexCoord = vTexCoord;
    
    // NEW: Transform world position to light space for shadow mapping
    outWorldPosLightSpace = gLightSpaceMatrix * worldPos_vec4;
}
