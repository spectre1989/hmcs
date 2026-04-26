#version 450

layout(push_constant) uniform push_t
{
    mat4 mvp;
} push;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_colour;

layout(location = 0) out vec3 out_colour;

void main() {
    gl_Position = push.mvp * vec4(in_position, 1.0);
    out_colour = in_colour;
}