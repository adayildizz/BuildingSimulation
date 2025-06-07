#version 410

layout (location = 0) in vec4 vPosition;   // Vertex position (model space)
layout (location = 1) in vec2 vTexCoord;
layout (location = 2) in vec3 vNormal;     // Vertex normal (model space)
layout (location = 3) in vec4 vColor;

uniform mat4 gVP;          // Combined View * Projection matrix
uniform mat4 gModelMatrix; // Model matrix (transforms model to world space)
uniform mat4 lightSpaceMatrix; // NEW: Light's (View * Projection) matrix


out vec4 baseColor;
out vec2 outTexCoord;      // Pass texture coordinates to fragment shader
out vec3 outWorldPos;      // Pass world position to fragment shader
out vec3 outNormal_world;  // Pass normal (in world space) to fragment shader
out vec4 outFragPosLightSpace; // NEW: Fragment position in light space


void main()
{
    // Transform vertex position to world space
    vec4 worldPos_vec4 = gModelMatrix * vec4(vPosition);
    outWorldPos = worldPos_vec4.xyz;

    // Transform vertex position to clip space
    gl_Position = gVP * worldPos_vec4;
    
    // Transform normal to world space
    outNormal_world = normalize(mat3(gModelMatrix) * vNormal);
    baseColor = vColor;
    outTexCoord = vTexCoord;
    
    // Transform fragment position to light space for shadow mapping
    outFragPosLightSpace = lightSpaceMatrix * worldPos_vec4;

}
