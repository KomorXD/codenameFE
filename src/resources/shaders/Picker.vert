#version 430 core

layout(location = 0) in vec3 a_Pos;
layout(location = 3) in mat4 a_Transform;
layout(location = 9) in float a_EntityID;

layout (std140, binding = 0) uniform Matrices
{
	mat4 u_Projection;
	mat4 u_View;
};

out VS_OUT
{
	vec3 worldPos;
	flat float entityID;
} vs_out;

void main()
{
	vs_out.worldPos	= (a_Transform * vec4(a_Pos, 1.0)).xyz;
	vs_out.entityID	= a_EntityID;

	gl_Position = u_Projection * u_View * a_Transform * vec4(a_Pos, 1.0);
}