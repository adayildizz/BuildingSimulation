#version 410

layout (location = 0) in vec3 vPosition;   // Vertex position (model space)
layout (location = 1) in vec2 vTexCoord;   // Texture coordinates
layout (location = 2) in vec3 vNormal;     // Vertex normal (model space)

uniform mat4 gVP;          // Combined View * Projection matrix
uniform mat4 gModelMatrix; // Model matrix (transforms model to world space)


out vec2 outTexCoord;       // Pass texture coordinates to fragment shader
out vec3 outWorldPos;       // Pass world position to fragment shader
out vec3 outNormal_world;   // Pass normal (in world space) to fragment shader - NEW

void main()
{
    // Transform vertex position to world space
    vec4 worldPos_vec4 = gModelMatrix * vec4(vPosition, 1.0);
    outWorldPos = worldPos_vec4.xyz;

    // Transform vertex position to clip space
    gl_Position = gVP * worldPos_vec4;
    
    // Transform normal to world space
    // For normals, use the upper 3x3 of the model matrix.
    outNormal_world = normalize(mat3(gModelMatrix) * vNormal);
    
    outTexCoord = vTexCoord;
    
}
