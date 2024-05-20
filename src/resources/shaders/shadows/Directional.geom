#version 430 core

layout(triangles, invocations = ${INVOCATIONS}) in;
layout(triangle_strip, max_vertices = ${MAX_VERTICES}) out;

struct DirectionalLight
{
	mat4 cascadeLightMatrices[${CASCADES_COUNT}];
	vec4 direction;
	vec4 color;
};

layout(std140, binding = 2) uniform DirectionalLights
{
	DirectionalLight lights[${MAX_DIR_LIGHTS}];
	int count;
} u_DirLights;

void main()
{
	if(gl_InvocationID >= u_DirLights.count)
	{
		return;
	}

	int layer = gl_InvocationID * ${CASCADES_COUNT};
	for(int cascade = 0; cascade < ${CASCADES_COUNT}; cascade++)
	{
		for(int v = 0; v < 3; v++)
		{
			gl_Layer = layer;
			gl_Position = u_DirLights.lights[gl_InvocationID].cascadeLightMatrices[cascade] * gl_in[v].gl_Position;
			EmitVertex();
		}

		layer++;
		EndPrimitive();
	}
}