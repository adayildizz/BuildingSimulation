#version 410

in vec4 baseColor;
in vec3 transformedNormal; // Normal in eye space, normalized
in vec2 vTexCoord;   // Interpolated texture coordinates

uniform sampler2D diffuseTexture; // Texture sampler

out vec4 fColor;

void main()
{
    // Lighting parameters
    vec3 lightDirEye = normalize(vec3(0.5, 0.5, 1.0)); // Directional light in eye space (e.g., top-right-front)
    vec3 lightColor = vec3(1.0, 1.0, 1.0);       // White light
    vec3 ambientColor = vec3(0.15, 0.15, 0.15);   // A bit of ambient light

    // Calculate diffuse lighting component
    float diffuseIntensity = max(dot(transformedNormal, lightDirEye), 0.0);
    vec3 diffuseColor = diffuseIntensity * lightColor;

    vec4 textureColor = texture(diffuseTexture, vTexCoord);
    
    // Combine ambient and diffuse light, then modulate with base object color
    vec3 finalColor = (ambientColor + diffuseColor) * baseColor.rgb;

    fColor = vec4(finalColor, baseColor.a);
    // Modulate texture color with material color and lighting
    // If texture has alpha, consider using it: fColor = vec4((ambient + diffuse) * textureColor.rgb * color.rgb, textureColor.a * color.a);
    // For now, assume opaque or blend with material alpha
    fColor = vec4((ambientColor + diffuseColor), 1.0) * textureColor * baseColor; 
    // Ensure final alpha is reasonable, e.g., from material color or texture alpha
    fColor.a = baseColor.a * textureColor.a; 
    if (fColor.a < 0.01) fColor.a = 1.0; // Prevent fully transparent if not intended
}
