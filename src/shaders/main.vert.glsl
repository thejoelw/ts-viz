#version 410 core

#defines

layout(location = POSITION_Y_LOCATION) in ELEMENT_TYPE position_y;

uniform vec2 offset;
uniform vec2 scale;

void main(void) {
    gl_Position = vec4(offset + vec2(gl_VertexID, position_y) * scale, 0.0, 1.0);
}
