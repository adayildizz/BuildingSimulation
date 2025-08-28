#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform vec3 u_color;
uniform bool u_hasTexture;
uniform sampler2D u_texture;

void main()
{
    if (u_hasTexture) {
        vec4 texColor = texture(u_texture, TexCoord);
        // Blend texture with color (tint the texture)
        FragColor = vec4(texColor.rgb * u_color, texColor.a);
    } else {
        FragColor = vec4(u_color, 1.0);
    }
}
