#version 430 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 48) out;

struct DirectionalLight
{
	vec4 direction;
	vec4 color;
};

struct PointLight
{
	vec4 positionAndLin;
	vec4 colorAndQuad;
};

struct SpotLight
{
	vec4 positionAndCutoff;
	vec4 directionAndOuterCutoff;
	vec4 colorAndLin;
	vec4 quadraticTerm;
};

layout(std140, binding = 2) uniform Lights
{
	DirectionalLight dirLights[128];
	PointLight pointLights[128];
	SpotLight spotLights[128];
	
	int dirLightsCount;
	int pointLightsCount;
	int spotLightsCount;
} lights;

uniform mat4 u_SpotlightMatrices[16];

void main()
{
	for(int i = 0; i < lights.spotLightsCount; i++)
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