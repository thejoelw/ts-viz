#version 410 core

#defines

uniform vec4 color;

layout(location = 0) out vec4 fragColor;

void main(void) {
    fragColor = color;
}
