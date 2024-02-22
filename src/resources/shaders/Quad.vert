#version 430 core

layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TextureUV;
layout(location = 3) in float a_TextureSlot;

layout (std140, binding = 0) uniform Matrices
{
	mat4 u_Projection;
	mat4 u_View;
};

out VS_OUT
{
	vec4 color;
	vec2 textureUV;
	flat float textureSlot;
} vs_out;

void main()
{
	vs_out.color	   = a_Color;
	vs_out.textureUV   = a_TextureUV;
	vs_out.textureSlot = a_TextureSlot;

	gl_Position = u_Projection * u_View * vec4(a_Pos, 1.0);
}