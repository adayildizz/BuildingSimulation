#version 410

layout(location=0) in vec3 vPosition;
layout(location=1) in vec2 vTexCoord;

uniform mat4 camMatrix;
uniform float gMinHeight;
uniform float gMaxHeight;

out vec4 fragmentColor;
out vec2 fragmentTexCoord;

void main()
{
    gl_Position = camMatrix * vec4(vPosition, 1.0);
    
    float deltaHeight = gMaxHeight - gMinHeight;
    float heightRatio = 0.0;

    if (deltaHeight > 0.001) {
        heightRatio = (vPosition.y - gMinHeight) / deltaHeight;
        heightRatio = clamp(heightRatio, 0.0, 1.0);
    }
    
    float c = heightRatio * 0.7 + 0.3;
    fragmentColor = vec4(c, c, c, 1.0f);
    
    fragmentTexCoord = vTexCoord;
}
