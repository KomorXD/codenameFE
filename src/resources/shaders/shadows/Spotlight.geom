#version 430 core

#define MAX_SPOTLIGHTS ${MAX_SPOTLIGHTS}

layout(triangles, invocations = ${INVOCATIONS}) in;
layout(triangle_strip, max_vertices = 3) out;

struct Spotlight
{
	mat4 lightSpaceMatrix;
	vec4 positionAndCutoff;
	vec4 directionAndOuterCutoff;
	vec4 colorAndLin;
	vec4 quadraticTerm;
};

layout(std140, binding = 4) uniform Spotlights
{
	Spotlight lights[MAX_SPOTLIGHTS];
	int count;
} u_Spotlights;

void main()
{
	if(gl_InvocationID >= u_Spotlights.count)
	{
		return;
	}

	for(int v = 0; v < 3; v++)
	{
		gl_Layer = gl_InvocationID;
		gl_Position = u_Spotlights.lights[gl_InvocationID].lightSpaceMatrix * gl_in[v].gl_Position;
		EmitVertex();
	}

	EndPrimitive();
}