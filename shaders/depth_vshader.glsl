#version 410

layout (location = 0) in vec4 vPosition; // Vertex position (model space)

uniform mat4 lightSpaceMatrix; // Light's (View * Projection) matrix
uniform mat4 gModelMatrix;     // Model matrix (transforms model to world space)

void main()
{
    // Transform vertex position to light's clip space
    gl_Position = lightSpaceMatrix * gModelMatrix * vPosition;
}
