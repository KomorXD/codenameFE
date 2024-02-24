#version 430 core

layout(location = 0) in vec3 a_Pos;
layout(location = 3) in mat4 a_Transform;

uniform mat4 u_LightMV;

void main()
{
	gl_Position = u_LightMV * a_Transform * vec4(a_Pos, 1.0);
}