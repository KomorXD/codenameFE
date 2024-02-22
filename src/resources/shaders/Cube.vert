#version 430 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TextureUV;
layout(location = 3) in mat4 a_Transform;
layout(location = 7) in vec4 a_Color;
layout(location = 8) in int  a_TextureSlot;

layout (std140, binding = 0) uniform Matrices
{
	mat4 u_Projection;
	mat4 u_View;
};

out VS_OUT
{
	vec3 worldPos;
	vec3 normal;
	vec4 color;
	vec2 textureUV;
	int  textureSlot;
} vs_out;

void main()
{
	vs_out.worldPos	   = (a_Transform * vec4(a_Pos, 1.0)).xyz;
	vs_out.normal	   = a_Normal;
	vs_out.color	   = a_Color;
	vs_out.textureUV   = a_TextureUV;
	vs_out.textureSlot = a_TextureSlot;

	gl_Position = u_Projection * u_View * a_Transform * vec4(a_Pos, 1.0);
}