#version 410

layout (location = 0) in vec4 vPosition;   // Vertex position (model space)
layout (location = 1) in vec2 vTexCoord;
layout (location = 2) in vec3 vNormal;     // Vertex normal (model space)
layout (location = 3) in vec4 vSplatWeights1234; // First 4 splat weights (sand, grass, dirt, rock)
layout (location = 4) in float vSplatWeight5;    // Fifth splat weight (snow)

uniform mat4 gVP;           // Combined View * Projection matrix
uniform mat4 gModelMatrix;  // Model matrix (transforms model to world space)
uniform mat4 gLightSpaceMatrix; // NEW: Transforms world to light space

uniform vec4 clipPlane; 


out vec2 outTexCoord;      // Pass texture coordinates to fragment shader
out vec3 outWorldPos;      // Pass world position to fragment shader
out vec3 outNormal_world;  // Pass normal (in world space) to fragment shader
out vec4 outSplatWeights1234; // Pass first 4 splat weights to fragment shader
out float outSplatWeight5;    // Pass fifth splat weight to fragment shader
out vec4 outWorldPosLightSpace; // NEW: Pass light-space position to fragment shader

void main()
{
    
    // Transform vertex position to world space
    vec4 worldPos_vec4 = gModelMatrix * vPosition; // Changed to use vPosition directly as a vec4
    outWorldPos = worldPos_vec4.xyz;
    
    // Transform vertex position to clip space
    gl_Position = gVP * worldPos_vec4;
    
    gl_ClipDistance[0] = dot(worldPos_vec4, clipPlane);
    
   // Transform normal to world space
    outNormal_world = normalize(mat3(transpose(inverse(gModelMatrix))) * vNormal);
    
    // Pass through texture coordinates and splat weights
    outTexCoord = vTexCoord;
    outSplatWeights1234 = vSplatWeights1234;
    outSplatWeight5 = vSplatWeight5;
 
    // NEW: Transform world position to light space for shadow mapping
    outWorldPosLightSpace = gLightSpaceMatrix * worldPos_vec4;
    
}
