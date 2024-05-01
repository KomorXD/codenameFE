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

uniform mat4 u_PointLightMatrices[16 * 6];

out GS_OUT
{
	vec3 fragPos;
	vec3 lightPos;
} gs_out;

void main()
{
	for(int i = 0; i < lights.pointLightsCount; i++)
	{
		gs_out.lightPos = lights.pointLights[i].positionAndLin.xyz;

		for(int face = 0; face < 6; face++)
		{
			for(int vert = 0; vert < 3; vert++)
			{
				gs_out.fragPos = gl_in[vert].gl_Position.xyz;
				gl_Layer = i * 6 + face;
				gl_Position = u_PointLightMatrices[i * 6 + face] * gl_in[vert].gl_Position;
				EmitVertex();
			}
			
			EndPrimitive();
		}
	}
}