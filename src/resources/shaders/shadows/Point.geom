#version 430 core

#define MAX_POINT_LIGHTS ${MAX_POINT_LIGHTS}

layout(triangles, invocations = ${INVOCATIONS}) in;
layout(triangle_strip, max_vertices = 18) out;

struct PointLight
{
	mat4 lightSpaceMatrices[6];
	vec4 renderedDirs[6];
	vec4 positionAndLin;
	vec4 colorAndQuad;
	int facesRendered;

	int pad1;
	int pad2;
	int pad3;
};

layout(std140, binding = 3) uniform PointLights
{
	PointLight lights[MAX_POINT_LIGHTS];
	int count;
} u_PointLights;

void main()
{
	if(gl_InvocationID >= u_PointLights.count)
	{
		return;
	}

	int layer = gl_InvocationID * 6;
	for(int face = 0; face < u_PointLights.lights[gl_InvocationID].facesRendered; face++)
	{
		for(int v = 0; v < 3; v++)
		{
			gl_Layer = layer;
			gl_Position = u_PointLights.lights[gl_InvocationID].lightSpaceMatrices[face] * gl_in[v].gl_Position;
			EmitVertex();
		}

		layer++;
		EndPrimitive();
	}
}