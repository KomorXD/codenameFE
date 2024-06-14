#version 430 core

layout (location = 0) out vec4 o_Color;
layout (location = 1) out vec4 o_Picker;

in vec2 textureUV;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColor;

void main()
{
	o_Color = texture(gColor, textureUV);

	int entID = int(texture(gPosition, textureUV).a);
	int rInt = int(mod(int(entID / 65025.0), 255));
	int gInt = int(mod(int(entID / 255.0), 255));
	int bInt = int(mod(entID, 255));
	float r = float(rInt) / 255.0;
	float g = float(gInt) / 255.0;
	float b = float(bInt) / 255.0;
	o_Picker = vec4(r, g, b, 1.0);
}