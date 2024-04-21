#version 430 core

layout(location = 0) in vec3 a_Pos;

out vec3 localPos;

layout (std140, binding = 0) uniform Matrices
{
	mat4 u_Projection;
	mat4 u_View;
};

void main()
{
	localPos = a_Pos;

	vec4 pos = u_Projection * mat4(mat3(u_View)) * vec4(a_Pos, 1.0);
	gl_Position = pos.xyww;
}
