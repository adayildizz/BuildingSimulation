#version 410

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vTexCoord;

uniform mat4 gVP;
uniform float gMinHeight;
uniform float gMaxHeight;

out vec4 outColor;
out vec2 outTexCoord;
out vec3 outWorldPos;

void main()
{
    gl_Position = gVP * vec4(vPosition, 1.0);
    
    float c = 0.5;
    if (gMaxHeight > gMinHeight) {
        float heightRatio = clamp((vPosition.y - gMinHeight) / (gMaxHeight - gMinHeight), 0.0, 1.0);
        c = heightRatio * 0.8 + 0.2;
    }
    
    outColor = vec4(c, c, c, 1.0);
    outTexCoord = vTexCoord;
    outWorldPos = vPosition;
}
