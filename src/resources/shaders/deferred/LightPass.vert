#version 430 core

layout(location = 0) in vec2 a_Pos;
layout(location = 1) in vec2 a_TextureUV;

out VS_OUT
{
	vec2 textureUV;
} vs_out;

void main()
{
	vs_out.textureUV = a_TextureUV;
	gl_Position = vec4(a_Pos.xy, 0.0, 1.0);
}