#version 430 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec4 a_Color;

layout (std140, binding = 0) uniform Camera
{
	mat4 projection;
	mat4 view;
	vec4 position;
	float exposure;
	float gamma;
	float near;
	float far;
} u_Camera;

out VS_OUT
{
	vec4 color;
} vs_out;

void main()
{
	vs_out.color = a_Color;

	gl_Position = u_Camera.projection * u_Camera.view * vec4(a_Pos, 1.0);
}