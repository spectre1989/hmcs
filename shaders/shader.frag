#version 450

layout(binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) in vec2 in_texcoords;

layout(location = 0) out vec4 out_colour;

void main() {
    out_colour = texture(texture_sampler, in_texcoords);
}