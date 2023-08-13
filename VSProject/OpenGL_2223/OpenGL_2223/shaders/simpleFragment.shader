#version 330 core
out vec4 FragColor;

in vec3 color;
in vec2 uv;
in mat3 tbn;
in vec3 worldPosition;

uniform sampler2D mainTex;
uniform sampler2D normalTex;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;

uniform vec3 ambientColor; // Ambient light color
uniform float ambientIntensity; // Intensity of the ambient light

void main()
{
    //Normal map
    vec3 normal = texture(normalTex, uv).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    normal.rg = normal.rg * 0.5f;
    normal = normalize(normal);
    normal = tbn * normal;

    vec3 lightDirection = normalize(worldPosition - lightPosition);

    //Specular data
    vec3 viewDir = normalize(worldPosition - cameraPosition);
    vec3 reflDir = normalize(reflect(lightDirection, normal));

    // Lighting
    float lightValue = max(-dot(normal, lightDirection), 0.0f);
    float specular = pow(max(-dot(reflDir, viewDir), 0.0), 8);

    // Ambient lighting calculations
    vec3 ambientLighting = ambientIntensity * ambientColor;

    vec4 output = vec4(color, 1.0f) * texture(mainTex, uv);
    output.rgb = output.rgb * min(lightValue + 0.1, 1.0f) + specular * output.rgb + ambientLighting;

    FragColor = output;
}