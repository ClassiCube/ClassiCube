#version 450

layout(location = 0) in vec3  vs_pos;
layout(location = 1) in vec4  vs_col;

layout(location = 0) out vec4 ps_col;

layout(push_constant) uniform Uniforms { mat4 mvp; } unis;

void main()
{
	gl_Position = unis.mvp * vec4(vs_pos, 1.0);
	ps_col = vs_col;
}

