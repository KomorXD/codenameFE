#version 430 core

in VS_OUT
{
	vec3 worldPos;
	flat float entityID;
} fs_in;

out vec4 fragColor;

void main()
{
	fragColor = vec4(vec3(fs_in.entityID), 1.0);
}