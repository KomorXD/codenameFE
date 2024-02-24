#version 430 core

layout(location = 0) in vec2 a_Pos;
layout(location = 1) in vec2 a_TextureUV;

out vec2 textureUV;

void main()
{
	textureUV = a_TextureUV;
	
	gl_Position = vec4(a_Pos.xy, 0.0, 1.0);
}