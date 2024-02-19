#version 430 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in mat4 a_Transform;
layout(location = 6) in vec4 a_Color;

uniform mat4 u_ViewProjection;

out vec3 normal;
out vec4 color;

void main()
{
	normal = a_Normal;
	color = a_Color;

	gl_Position = u_ViewProjection * a_Transform * vec4(a_Pos, 1.0);
}