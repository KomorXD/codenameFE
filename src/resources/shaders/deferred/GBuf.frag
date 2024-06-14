#version 430 core

#define MATERIALS_COUNT ${MATERIALS_COUNT}
#define TEXTURE_UNITS ${TEXTURE_UNITS}

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gColor;
layout (location = 3) out vec4 gMaterial;

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

vec2 heightMapUV(vec2 texCoords, vec3 viewDir, sampler2D depthMap, float heightScale, bool isDepthMap)
{
	const float minLayers = 8.0;
	const float maxLayers = 64.0;
	float layers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));

	float layerDepth = 1.0 / layers;
	float currentDepth = 0.0;
	vec2 p = vec2(viewDir.x, -viewDir.y) * heightScale;
	vec2 deltaCoords = p / layers;

	vec2 currentCoords = texCoords;
	float depthMapValue = texture(depthMap, currentCoords).r;
	
	for(int i = 0; i < 16; i++)
	{
		if(currentDepth < depthMapValue)
		{
			break;
		}
		currentCoords -= deltaCoords;
		depthMapValue = texture(depthMap, currentCoords).r;
		currentDepth += layerDepth;
	}

	vec2 prevCoords = currentCoords + deltaCoords;
	float afterDepth = depthMapValue - currentDepth;
	if(isDepthMap)
	{
		afterDepth = 1.0 - depthMapValue - currentDepth;
	}

	float beforeRead = texture(depthMap, prevCoords).r;
	float beforeDepth = beforeRead - currentDepth + layerDepth;
	if(isDepthMap)
	{
		beforeDepth = 1.0 - beforeRead - currentDepth + layerDepth;
	}

	float weight = afterDepth / (afterDepth - beforeDepth);

	return prevCoords * weight + currentCoords * (1.0 - weight);
}

void main()
{
	gPosition = vec4(fs_in.worldPos, fs_in.entityID);
	
	Material mat = u_Materials.materials[int(fs_in.materialSlot)];
	vec2 texCoords = fs_in.textureUV * mat.tilingFactor + mat.texOffset;
	vec3 V = normalize(fs_in.tangentViewPos - fs_in.tangentWorldPos);
	texCoords = heightMapUV(texCoords, V, u_Textures[mat.heightTextureSlot], mat.heightFactor, bool(mat.isDepthMap));
	gColor = texture(u_Textures[mat.albedoTextureSlot], texCoords) * mat.color;

	vec3 N = texture(u_Textures[mat.normalTextureSlot], texCoords).rgb;
	N = N * 2.0 - 1.0;
	gNormal = vec4(normalize(transpose(fs_in.TBN) * N), 1.0);

	float roughness = texture(u_Textures[mat.roughnessTextureSlot], texCoords).r * mat.roughnessFactor;
	float metallic = texture(u_Textures[mat.metallicTextureSlot], texCoords).r * mat.metallicFactor;
	float AO = texture(u_Textures[mat.ambientOccTextureSlot], texCoords).r * mat.ambientOccFactor;
	gMaterial = vec4(roughness, metallic, AO, 1.0);
}