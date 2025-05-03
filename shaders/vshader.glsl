#version 410

in vec4 vPosition;
out vec4 color;

uniform mat4 camMatrix;

void main()
{
    gl_Position = camMatrix * vPosition;
    color = vec4(1.0);
}
