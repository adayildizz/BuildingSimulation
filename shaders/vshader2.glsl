#version 410

in vec4 vPosition;
in vec3 vNormal; // Changed from vec4, as normals are 3D vectors
in vec4 vColor;
in vec2 vTexCoord_attr; // Texture coordinates from VBO

out vec4 baseColor;
out vec3 transformedNormal;
out vec2 vTexCoord;     // Pass texture coordinates to fragment shader

uniform mat4 ModelView;
uniform mat4 Projection;

void main()
{
    gl_Position = Projection * ModelView * vPosition;
    // Transform normal to eye space. Use mat3(ModelView) for correct normal transformation
    // (handles non-uniform scaling better, though for uniform scale and rotation it's similar to mat4)
    transformedNormal = normalize(mat3(ModelView) * vNormal);
    baseColor = vColor;
    vTexCoord = vTexCoord_attr;
}
