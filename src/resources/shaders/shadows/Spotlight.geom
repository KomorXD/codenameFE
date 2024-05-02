#version 430 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 48) out;

struct Spotlight
{
	vec4 positionAndCutoff;
	vec4 directionAndOuterCutoff;
	vec4 colorAndLin;
	vec4 quadraticTerm;
};

layout(std140, binding = 4) uniform Spotlights
{
	Spotlight lights[128];
	int count;
} u_Spotlights;

uniform mat4 u_SpotlightMatrices[16];

void main()
{
	for(int i = 0; i < u_Spotlights.count; i++)
	{
		for(int j = 0; j < 3; j++)
		{
			gl_Layer = i;
			gl_Position = u_SpotlightMatrices[i] * gl_in[j].gl_Position;
			EmitVertex();
		}

		EndPrimitive();
	}
}