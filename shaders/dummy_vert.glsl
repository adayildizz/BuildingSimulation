#version 330 core
layout(location = 0) in vec4 vPosition;   // Vertex position (model space)
layout(location = 1) in vec2 vTexCoord;   // Texture coordinates

uniform mat4 gVP;          // Combined View * Projection matrix
uniform mat4 gModelMatrix; // Model matrix (transforms model to world space)

out vec2 fTexCoord;

void main() {
    // Transform vertex position to clip space
    gl_Position = gVP * gModelMatrix * vPosition;
    fTexCoord = vTexCoord;
}
