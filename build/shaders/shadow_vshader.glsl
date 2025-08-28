#version 410

layout (location = 0) in vec4 vPosition;

uniform mat4 gLightSpaceMatrix;
uniform mat4 gModelMatrix;

void main()
{
    gl_Position = gLightSpaceMatrix * gModelMatrix * vPosition;
}
