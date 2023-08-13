#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 modelMatrix;           // Model transformation matrix
uniform mat4 viewProjectionMatrix;  // View-projection matrix
uniform mat4 previousViewProjectionMatrix; // Previous frame's view-projection matrix

out vec2 fragCoord;

void main()
{
    // Calculate the current vertex position in world space
    vec4 worldPosition = modelMatrix * vec4(aPos, 1.0);

    // Calculate the previous vertex position in clip space using the previous view-projection matrix
    vec4 previousClipPosition = previousViewProjectionMatrix * worldPosition;

    // Calculate the current vertex position in clip space using the current view-projection matrix
    gl_Position = viewProjectionMatrix * worldPosition;

    // Calculate the velocity of the vertex using the difference between current and previous clip positions
    vec2 velocity = (gl_Position.xy / gl_Position.w - previousClipPosition.xy / previousClipPosition.w) * 0.5;

    // Pass the velocity as a varying to the fragment shader
    fragCoord = velocity;
}
