#version 330 core
in vec2 fTexCoord;
out vec4 fragColor;

void main() {
    fragColor = vec4(fTexCoord, 0.5, 1.0); // Visual gradient
}
