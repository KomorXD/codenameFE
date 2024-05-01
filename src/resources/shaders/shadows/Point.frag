#version 430 core

in GS_OUT
{
	vec3 fragPos;
	vec3 lightPos;
} fs_in;

void main()
{
	gl_FragDepth = length(fs_in.fragPos - fs_in.lightPos) / 1000.0;
}