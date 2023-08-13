#version 330 core

uniform sampler2D depthTexture;    // Depth buffer texture
uniform sampler2D sceneSampler;    // Scene color texture
uniform mat4 g_ViewProjectionInverseMatrix;   // Inverse of current view-projection matrix
uniform mat4 g_previousViewProjectionMatrix;  // Previous frame's view-projection matrix
uniform int g_numSamples;    // Number of samples for motion blur

in vec2 fragCoord;    // Input texture coordinate

out vec4 FragColor;   // Output color

void main()
{
    // Get the depth buffer value at this pixel.
    float zOverW = texture(depthTexture, fragCoord).r;

    // Calculate the viewport position at this pixel in the range -1 to 1.
    vec4 H = vec4(fragCoord.x * 2.0 - 1.0, 1.0 - fragCoord.y * 2.0, zOverW, 1.0);

    // Transform by the view-projection inverse matrix.
    vec4 D = g_ViewProjectionInverseMatrix * H;

    // Divide by w to get the world position.
    vec4 worldPos = D / D.w;

    // Calculate the current viewport position.
    vec4 currentPos = H;

    // Transform the world position by the previous view-projection matrix.
    vec4 previousPos = g_previousViewProjectionMatrix * worldPos;

    // Convert to nonhomogeneous points [-1,1] by dividing by w.
    previousPos /= previousPos.w;

    // Calculate the pixel velocity.
    vec2 velocity = (currentPos.xy - previousPos.xy) / 2.0;
    velocity.x *= abs(g_ViewProjectionInverseMatrix[3][0]);
    velocity.y *= abs(g_ViewProjectionInverseMatrix[3][1]);

    // Get the initial color at this pixel.
    vec4 color = texture(sceneSampler, fragCoord);

    // Accumulate colors for motion blur
    vec4 accumulatedColor = vec4(0.0);

    // Sample along the pixel's motion path and accumulate colors
    for (int i = 0; i < g_numSamples; ++i)
    {
        vec2 sampleCoord = fragCoord + velocity * float(i) / float(g_numSamples - 1);
        accumulatedColor += texture(sceneSampler, sampleCoord);
    }

    // Calculate the final color by averaging the accumulated colors
    vec4 finalColor = accumulatedColor / float(g_numSamples);

    FragColor = finalColor;
}