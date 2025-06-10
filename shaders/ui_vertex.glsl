#version 330 core
layout (location = 0) in vec2 aPos;

uniform mat4 u_projection;
uniform mat4 u_model;

out vec2 TexCoord;

void main()
{
    gl_Position = u_projection * u_model * vec4(aPos, 0.0, 1.0);
    TexCoord = aPos; // Pass texture coordinates (0,0 to 1,1)
}
