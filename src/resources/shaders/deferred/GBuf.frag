#version 430 core

#define MATERIALS_COUNT ${MATERIALS_COUNT}
#define TEXTURE_UNITS ${TEXTURE_UNITS}

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gColor;

struct Material
{
	vec4 color;
	vec2 tilingFactor;
	vec2 texOffset;

	int albedoTextureSlot;
	int normalTextureSlot;

	int heightTextureSlot;
	float heightFactor;
	int isDepthMap;

	int	roughnessTextureSlot;
	float roughnessFactor;

	int metallicTextureSlot;
	float metallicFactor;

	int ambientOccTextureSlot;
	float ambientOccFactor;

	float padding;
};

layout(std140, binding = 1) uniform Materials
{
	Material materials[MATERIALS_COUNT];
} u_Materials;

uniform sampler2D u_Textures[TEXTURE_UNITS];

in VS_OUT
{
	vec3 worldPos;
	vec3 viewSpacePos;
	vec3 eyePos;
	vec3 normal;
	mat3 TBN;
	vec3 tangentWorldPos;
	vec3 tangentViewPos;
	vec2 textureUV;
	flat float materialSlot;
	flat float entityID;
} fs_in;

void main()
{
	gPosition = vec4(fs_in.worldPos, fs_in.entityID);
	
	Material mat = u_Materials.materials[int(fs_in.materialSlot)];
	vec3 N = texture(u_Textures[mat.normalTextureSlot], fs_in.textureUV).rgb;
	N = N * 2.0 - 1.0;
	gNormal = vec4(transpose(fs_in.TBN) * normalize(N), 1.0);

	gColor = texture(u_Textures[mat.albedoTextureSlot], fs_in.textureUV) * mat.color;
}