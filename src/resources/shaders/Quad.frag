#version 430 core

in VS_OUT
{
	vec4 color;
} fs_in;

out vec4 fragColor;

void main()
{
	fragColor = fs_in.color;
}