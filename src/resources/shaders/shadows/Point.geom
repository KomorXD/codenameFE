#version 430 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 48) out;

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
	PointLight lights[64];
	int count;
} u_PointLights;

void main()
{
	int layer = 0;
	for(int i = 0; i < u_PointLights.count; i++)
	{
		for(int face = 0; face < u_PointLights.lights[i].facesRendered; face++)
		{
			for(int j = 0; j < 3; j++)
			{
				gl_Layer = layer;
				gl_Position = u_PointLights.lights[i].lightSpaceMatrices[face] * gl_in[j].gl_Position;
				EmitVertex();
			}

			layer++;
			EndPrimitive();
		}
	}
}