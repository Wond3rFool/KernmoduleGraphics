#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normals;
in vec4 FragPos;

uniform sampler2D diffuse;

uniform vec3 cameraPosition;
uniform vec3 lightDirection;

uniform float time;

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

vec4 lerp(vec4 a, vec4 b, float t)
{
    return a + (b - a) * t;
}

vec3 lerp(vec3 a, vec3 b, float t)
{
    return a + (b - a) * t;
}
vec2 lerp(vec2 a, vec2 b, float t)
{
    return a + (b - a) * t;
}

void main()
{
    
    vec4 diffuseColor = texture(diffuse, TexCoords);

    float light = max(dot(-lightDirection, Normals) + .25, 0.0);
    light = pow(light * 128, 2.0) / 128.0;
    light = max( min(light,1.0), 0.0);


    vec3 viewDir = normalize(FragPos.rgb - cameraPosition);
    vec3 refl = reflect(lightDirection, Normals);

    float spec = pow(max(dot(-viewDir, refl), 0.0), 2.0);
    
    vec3 specular = spec  * vec3(.2, .2, .2);


    vec4 output = diffuseColor * light + vec4(specular, 0);
    
    FragColor = output;
}