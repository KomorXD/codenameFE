#version 430 core

in GS_OUT
{
	vec3 fragPos;
	vec3 lightPos;
} fs_in;

void main()
{
	vec3 dir = fs_in.fragPos - fs_in.lightPos;
	gl_FragDepth = dot(dir, dir) / 1000.0;
}