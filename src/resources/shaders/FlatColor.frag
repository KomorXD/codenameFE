#version 430 core

#define MATERIALS_COUNT ${MATERIALS_COUNT}
#define TEXTURE_UNITS ${TEXTURE_UNITS}

layout (location = 0) out vec4 o_Color;
layout (location = 1) out vec4 o_Picker;

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
	vec2 textureUV;
	flat float materialSlot;
	flat float entityID;
} fs_in;

void main()
{
	Material mat = u_Materials.materials[int(fs_in.materialSlot)];
	vec2 texCoords = fs_in.textureUV * mat.tilingFactor + mat.texOffset;
	o_Color = texture(u_Textures[mat.albedoTextureSlot], texCoords) * mat.color;

	int entID = int(fs_in.entityID);
	int rInt = int(mod(int(entID / 65025.0), 255));
	int gInt = int(mod(int(entID / 255.0), 255));
	int bInt = int(mod(entID, 255));
	float r = float(rInt) / 255.0;
	float g = float(gInt) / 255.0;
	float b = float(bInt) / 255.0;
	o_Picker = vec4(r, g, b, 1.0);
}