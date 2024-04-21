#version 430 core

out vec4 fragColor;

in vec3 localPos;

uniform samplerCube u_Cubemap;

void main()
{
	vec3 color = texture(u_Cubemap, localPos).rgb;
	fragColor = vec4(color, 1.0);
}