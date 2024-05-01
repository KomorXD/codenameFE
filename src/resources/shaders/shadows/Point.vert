#version 430 core

layout (location = 0) in vec3 a_Pos;
layout (location = 5) in mat4 a_Transform;

void main()
{
	gl_Position = a_Transform * vec4(a_Pos, 1.0);
}