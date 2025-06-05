#version 330 core

layout(location = 0) in vec4 vPosition;  
layout(location = 1) in vec2 vTexCoord;   

uniform float uTime;
uniform mat4 ModelView;
uniform mat4 Projection;

out vec2 fTexCoord;
out vec4 clipSpace;

const float tiling = 6.0;

void main() {
    float wave = 0.005 * sin(10.0 * vPosition.x + uTime * 2.0)
               + 0.005 * cos(10.0 * vPosition.z + uTime * 1.5);

    vec4 displaced = vPosition + vec4(0.0, wave, 0.0, 0.0);

    clipSpace = Projection * ModelView * displaced;
    gl_Position = clipSpace;
    fTexCoord = vec2(vPosition.x / 2.0 + 0.5, vPosition.z / 2.0 + 0.5) * tiling;
}

