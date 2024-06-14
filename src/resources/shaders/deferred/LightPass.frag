#version 430 core

layout (location = 0) out vec4 o_Color;

in vec2 textureUV;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColor;

void main()
{
	o_Color = texture(gColor, textureUV);
}