#version 430 core

in vec3 normal;
in vec4 color;

out vec4 fragColor;

const float AMBIENT_STRENGTH = 0.1;
const vec3 DIRECTIONAL_DIR = normalize(vec3(-0.3, -0.5, -0.6));

void main()
{
	float directionalDiffuse = max(dot(normal, -DIRECTIONAL_DIR), 0.0);
	
	fragColor.rgb = (AMBIENT_STRENGTH + directionalDiffuse) * color.rgb;
	fragColor.rgb = pow(fragColor.rgb, vec3(1.0 / 2.2));
	fragColor.a = 1.0;
}