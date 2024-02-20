#version 430 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in mat4 a_Transform;
layout(location = 6) in vec4 a_Color;

uniform mat4 u_ViewProjection;

out VS_OUT
{
	vec3 worldPos;
	vec3 normal;
	vec4 color;
} vs_out;

void main()
{
	vs_out.worldPos = (a_Transform * vec4(a_Pos, 1.0)).xyz;
	vs_out.normal = a_Normal;
	vs_out.color = a_Color;

	gl_Position = u_ViewProjection * a_Transform * vec4(a_Pos, 1.0);
}