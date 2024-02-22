#version 430 core

in VS_OUT
{
	vec4 color;
	vec2 textureUV;
	flat float textureSlot;
} fs_in;

uniform sampler2D u_Textures[24];

out vec4 fragColor;

void main()
{
	int slot = -1;
	switch(int(fs_in.textureSlot))
	{
		case  0: slot = 0;  break;
		case  1: slot = 1;  break;
		case  2: slot = 2;  break;
		case  3: slot = 3;  break;
		case  4: slot = 4;  break;
		case  5: slot = 5;  break;
		case  6: slot = 6;  break;
		case  7: slot = 7;  break;
		case  8: slot = 8;  break;
		case  9: slot = 9;  break;
		case 10: slot = 10; break;
		case 11: slot = 11; break;
		case 12: slot = 12; break;
		case 13: slot = 13; break;
		case 14: slot = 14; break;
		case 15: slot = 15; break;
		case 16: slot = 16; break;
		case 17: slot = 17; break;
		case 18: slot = 18; break;
		case 19: slot = 19; break;
		case 20: slot = 20; break;
		case 21: slot = 21; break;
		case 22: slot = 22; break;
		case 23: slot = 23; break;
	}

	fragColor = (slot < 0 ? fs_in.color : texture(u_Textures[slot], fs_in.textureUV));
}