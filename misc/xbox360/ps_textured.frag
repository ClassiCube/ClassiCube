#version 450

layout(location = 0) in vec4 ps_col;
layout(location = 1) in vec2 ps_tex;

layout(location = 0) out vec4 out_col;

layout(set = 0, binding = 0) uniform sampler2D ps_sampler;

void main()
{
	out_col = ps_col * texture(ps_sampler, ps_tex);
}
