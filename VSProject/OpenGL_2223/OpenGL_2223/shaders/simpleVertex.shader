#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 vColor;
layout(location = 2) in vec2 vUV;
layout(location = 3) in vec3 vNormal;
layout(location = 4) in vec3 vTangent;
layout(location = 5) in vec3 vBitangent;

out vec3 color;
out vec2 uv;
out mat3 tbn;
out vec3 worldPosition;

uniform mat4 world, view, projection;

uniform float time; // Time parameter for animation

void main() 
{
	// Apply a simple vertical oscillation to the Y-coordinate of the vertex
	float oscillationAmplitude = 0.1; // Adjust the amplitude of the oscillation
	vec3 animatedPos = aPos + vec3(0.0, oscillationAmplitude * sin(time), 0.0);

	gl_Position = projection * view * world * vec4(animatedPos, 1.0);

	color = vColor;
	uv = vUV;

	vec3 t = normalize(mat3(world) * vTangent);
	vec3 b = normalize(mat3(world) * vBitangent);
	vec3 n = normalize(mat3(world) * vNormal);

	tbn = mat3(t, b, n);
	worldPosition = mat3(world) * aPos;


}