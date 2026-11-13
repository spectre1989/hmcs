#version 450

layout(push_constant) uniform push_t
{
    mat4 mvp;
} push;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texcoords;

layout(location = 0) out vec2 out_texcoords;

void main() {
    gl_Position = push.mvp * vec4(in_position, 1.0);
    out_texcoords = in_texcoords;
}