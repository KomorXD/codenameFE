#version 430 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 48) out;

struct DirectionalLight
{
	mat4 cascadeLightMatrices[5];
	vec4 direction;
	vec4 color;
};

layout(std140, binding = 2) uniform DirectionalLights
{
	DirectionalLight lights[128];
	int count;
} u_DirLights;

void main()
{
	int layer = 0;
	for(int i = 0; i < 1; i++)
	{
		for(int cascade = 0; cascade < 5; cascade++)
		{
			for(int j = 0; j < 3; j++)
			{
				gl_Layer = layer;
				gl_Position = u_DirLights.lights[i].cascadeLightMatrices[cascade] * gl_in[j].gl_Position;
				EmitVertex();
			}

			layer++;
			EndPrimitive();
		}
	}
}