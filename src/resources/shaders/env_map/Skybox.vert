#version 430 core

layout(location = 0) in vec3 a_Pos;

out vec3 localPos;

layout (std140, binding = 0) uniform Camera
{
	mat4 projection;
	mat4 view;
	vec4 position;
	vec2 screenSize;
	float exposure;
	float gamma;
	float near;
	float far;
} u_Camera;

void main()
{
	localPos = a_Pos;

	vec4 pos = u_Camera.projection * mat4(mat3(u_Camera.view)) * vec4(a_Pos, 1.0);
	gl_Position = pos.xyww;
}
