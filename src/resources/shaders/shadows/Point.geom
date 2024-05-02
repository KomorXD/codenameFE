#version 430 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 48) out;

struct PointLight
{
	vec4 positionAndLin;
	vec4 colorAndQuad;
};

layout(std140, binding = 3) uniform PointLights
{
	PointLight lights[128];
	int count;
} u_PointLights;

uniform mat4 u_PointLightMatrices[16 * 6];

out GS_OUT
{
	vec3 fragPos;
	vec3 lightPos;
} gs_out;

void main()
{
	for(int i = 0; i < u_PointLights.count; i++)
	{
		gs_out.lightPos = u_PointLights.lights[i].positionAndLin.xyz;

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