#version 410

layout(location = 0) out vec4 outputColor;

in vec4 fragmentColor;
in vec2 fragmentTexCoord;

void main()
{
     outputColor = fragmentColor;
}