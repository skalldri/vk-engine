#version 450

// Write outColor to attachment in location 0
// This defines where the final image is written to
layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 fragColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}