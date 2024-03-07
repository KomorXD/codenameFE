#version 430 core

in VS_OUT
{
	vec3 worldPos;
	flat float entityID;
} fs_in;

out vec4 fragColor;

void main()
{
	int entID = int(fs_in.entityID);

	int rInt = int(mod(int(entID / 65025.0), 255));
	int gInt = int(mod(int(entID / 255.0), 255));
	int bInt = int(mod(entID, 255));

	float r = float(rInt) / 255.0;
	float g = float(gInt) / 255.0;
	float b = float(bInt) / 255.0;
	
	fragColor = vec4(r, g, b, 1.0);
}