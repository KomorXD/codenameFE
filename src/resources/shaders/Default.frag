#version 430 core

#define MATERIALS_COUNT ${MATERIALS_COUNT}
#define TEXTURE_UNITS ${TEXTURE_UNITS}
#define MAX_DIR_LIGHTS ${MAX_DIR_LIGHTS}
#define MAX_POINT_LIGHTS ${MAX_POINT_LIGHTS}
#define MAX_SPOTLIGHTS ${MAX_SPOTLIGHTS}

layout(location = 0) out vec4 gDefault;
layout(location = 1) out vec4 gPicker;

struct DirectionalLight
{
	mat4 cascadeLightMatrices[${CASCADES_COUNT}];
	vec4 direction;
	vec4 color;
};

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

struct Spotlight
{
	mat4 lightSpaceMatrix;
	vec4 positionAndCutoff;
	vec4 directionAndOuterCutoff;
	vec4 colorAndLin;
	vec4 quadraticTerm;
};

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

layout (std140, binding = 0) uniform Camera
{
	mat4 projection;
	mat4 view;
	vec4 position;
	vec2 screenSize;
	float exposure;
	float gamma;
	float near;
	float far;
} u_Camera;

layout(std140, binding = 1) uniform Materials
{
	Material materials[MATERIALS_COUNT];
} u_Materials;

layout(std140, binding = 2) uniform DirectionalLights
{
	DirectionalLight lights[MAX_DIR_LIGHTS];
	int count;
} u_DirLights;

layout(std140, binding = 3) uniform PointLights
{
	PointLight lights[MAX_POINT_LIGHTS];
	int count;
} u_PointLights;

layout(std140, binding = 4) uniform Spotlights
{
	Spotlight lights[MAX_SPOTLIGHTS];
	int count;
} u_Spotlights;

uniform bool u_IsLightSource = false;
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_PrefilterMap;
uniform sampler2D u_BRDF_LUT;
uniform sampler2D u_Textures[TEXTURE_UNITS];

uniform float u_CascadeDistances[${CASCADES_COUNT}];
uniform sampler2DArrayShadow u_DirLightCSM;
uniform sampler2DArrayShadow u_PointLightShadowmaps;
uniform sampler2DArrayShadow u_SpotlightShadowmaps;

uniform int u_OffsetsTexSize;
uniform int u_OffsetsFilterSize;
uniform float u_OffsetsRadius;
uniform sampler3D u_OffsetsTexture;

uniform bool u_FasterShadows = true;

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float distGGX(vec3 N, vec3 H, float roughness)
{
	float a	  = roughness * roughness;
	float a2  = a * a;
	float NH  = max(dot(N, H), 0.0);
	float NH2 = NH * NH;
	float denom = (NH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return a2 / denom;
}

float geoSchlickGGX(float NV, float roughness)
{
	float a = roughness + 1.0;
	float k = (a * a) / 8.0;
	float denom = NV * (1.0 - k) + k;

	return NV / denom;
}

float geoSmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NV = max(dot(N, V), 0.0);
	float NL = max(dot(N, L), 0.0);

	float ggx2 = geoSchlickGGX(NV, roughness);
	float ggx1 = geoSchlickGGX(NL, roughness);

	return ggx1 * ggx2;
}

vec2 heightMapUV(vec2 texCoords, vec3 viewDir, sampler2D depthMap, float heightScale)
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
	float beforeDepth = texture(depthMap, prevCoords).r - currentDepth + layerDepth;
	float weight = afterDepth / (afterDepth - beforeDepth);

	return prevCoords * weight + currentCoords * (1.0 - weight);
}

vec2 depthMapUV(vec2 texCoords, vec3 viewDir, sampler2D depthMap, float heightScale)
{
	const float minLayers = 8.0;
	const float maxLayers = 64.0;
	float layers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));

	float layerDepth = 1.0 / layers;
	float currentDepth = 0.0;
	vec2 p = vec2(viewDir.x, -viewDir.y) * heightScale;
	vec2 deltaCoords = p / layers;

	vec2 currentCoords = texCoords;
	float depthMapValue = 1.0 - texture(depthMap, currentCoords).r;

	for(int i = 0; i < 16; i++)
	{
		if(currentDepth < depthMapValue)
		{
			break;
		}

		currentCoords -= deltaCoords;
		depthMapValue = 1.0 - texture(depthMap, currentCoords).r;
		currentDepth += layerDepth;
	}

	vec2 prevCoords = currentCoords + deltaCoords;
	float afterDepth = depthMapValue - currentDepth;
	float beforeDepth = 1.0 - texture(depthMap, prevCoords).r - currentDepth + layerDepth;
	float weight = afterDepth / (afterDepth - beforeDepth);

	return prevCoords * weight + currentCoords * (1.0 - weight);
}

vec3 offsets[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float cascadedShadowFactor(int dirLightIdx, vec3 N, vec3 L)
{
	vec4 fragPosViewSpace = u_Camera.view * vec4(fs_in.worldPos, 1.0);
	float depth = abs(fragPosViewSpace.z);
	int layer = -1;
	for(int i = 0; i < 5; i++)
	{
		if(depth < u_CascadeDistances[i])
		{
			layer = i;
			break;
		}
	}

	if(layer == -1)
	{
		return 1.0;
	}

	vec4 fragPosLightSpace = u_DirLights.lights[dirLightIdx].cascadeLightMatrices[layer] * vec4(fs_in.worldPos, 1.0);
	float bias = max(0.005 * (1.0 - dot(N, L)), 0.0005);
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;
	projCoords.z -= bias;

	float currentDepth = projCoords.z;
	if(currentDepth > 1.0)
	{
		return 0.0;
	}
	
	vec2 f = mod(gl_FragCoord.xy, vec2(u_OffsetsTexSize));
	ivec3 offsetCoord;
	offsetCoord.yz = ivec2(f);

	int samplesDiv2 = int(u_OffsetsFilterSize * u_OffsetsFilterSize / 2.0);
	vec4 sc = vec4(projCoords, 1.0);
	const vec2 texelSize = 1.0 / textureSize(u_DirLightCSM, 0).xy;

	float shadow = 0.0;
	for(int i = 0; i < 4; i++)
	{
		offsetCoord.x = i;
		vec4 offsets = texelFetch(u_OffsetsTexture, offsetCoord, 0) * u_OffsetsRadius;

		sc.xy = projCoords.xy + offsets.rg * texelSize;
		depth = texture(u_DirLightCSM, vec4(sc.xy, dirLightIdx + layer, projCoords.z));
		shadow += depth;

		sc.xy = projCoords.xy + offsets.ba * texelSize;
		depth = texture(u_DirLightCSM, vec4(sc.xy, dirLightIdx + layer, projCoords.z));
		shadow += depth;
	}

	shadow /= 8.0;
	if(shadow == 0.0 || shadow == 1.0)
	{
		return shadow;
	}

	shadow *= 8.0;
	for(int i = 4; i < samplesDiv2; i++)
	{
		offsetCoord.x = i;
		vec4 offsets = texelFetch(u_OffsetsTexture, offsetCoord, 0) * u_OffsetsRadius;

		sc.xy = projCoords.xy + offsets.rg * texelSize;
		depth = texture(u_DirLightCSM, vec4(sc.xy, dirLightIdx + layer, projCoords.z));
		shadow += depth;

		sc.xy = projCoords.xy + offsets.ba * texelSize;
		depth = texture(u_DirLightCSM, vec4(sc.xy, dirLightIdx + layer, projCoords.z));
		shadow += depth;
	}

	shadow /= float(samplesDiv2) * 2.0;
	return shadow;
}

float shadowFactor(sampler2DArrayShadow shadowMaps, mat4 lightSpaceMat, int layer, vec3 N, vec3 L)
{
	vec4 fragPosLightSpace = lightSpaceMat * vec4(fs_in.worldPos, 1.0);
	float bias = max(0.005 * (1.0 - dot(N, L)), 0.00005);
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;
	projCoords.z -= bias;

	float currentDepth = projCoords.z;
	if(currentDepth > 1.0)
	{
		return 0.0;
	}
	
	vec2 f = mod(gl_FragCoord.xy, vec2(u_OffsetsTexSize));
	ivec3 offsetCoord;
	offsetCoord.yz = ivec2(f);

	int samplesDiv2 = int(u_OffsetsFilterSize * u_OffsetsFilterSize / 2.0);
	vec4 sc = vec4(projCoords, 1.0);
	const vec2 texelSize = 1.0 / textureSize(shadowMaps, 0).xy;

	float depth = 0.0;
	float shadow = 0.0;
	for(int i = 0; i < 4; i++)
	{
		offsetCoord.x = i;
		vec4 offsets = texelFetch(u_OffsetsTexture, offsetCoord, 0) * u_OffsetsRadius;

		sc.xy = projCoords.xy + offsets.rg * texelSize;
		depth = texture(shadowMaps, vec4(sc.xy, layer, projCoords.z));
		shadow += depth;

		sc.xy = projCoords.xy + offsets.ba * texelSize;
		depth = texture(shadowMaps, vec4(sc.xy, layer, projCoords.z));
		shadow += depth;
	}

	shadow /= 8.0;
	if(shadow == 0.0 || shadow == 1.0)
	{
		return shadow;
	}

	shadow *= 8.0;
	for(int i = 4; i < samplesDiv2; i++)
	{
		offsetCoord.x = i;
		vec4 offsets = texelFetch(u_OffsetsTexture, offsetCoord, 0) * u_OffsetsRadius;

		sc.xy = projCoords.xy + offsets.rg * texelSize;
		depth = texture(shadowMaps, vec4(sc.xy, layer, projCoords.z));
		shadow += depth;

		sc.xy = projCoords.xy + offsets.ba * texelSize;
		depth = texture(shadowMaps, vec4(sc.xy, layer, projCoords.z));
		shadow += depth;
	}

	shadow /= float(samplesDiv2) * 2.0;
	return shadow;
}

float worseShadowFactor(sampler2DArrayShadow shadowMaps, mat4 lightSpaceMat, int layer, vec3 N, vec3 L)
{
	vec4 fragPosLightSpace = lightSpaceMat * vec4(fs_in.worldPos, 1.0);
	float bias = max(0.005 * (1.0 - dot(N, L)), 0.00005);
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;
	projCoords.z -= bias;

	float currentDepth = projCoords.z;
	if(currentDepth > 1.0)
	{
		return 0.0;
	}

	const vec2 texelSize = 1.0 / textureSize(shadowMaps, 0).xy;
	float shadow = 0.0;
	for(int i = 0; i < 20; i++)
	{
		vec2 texCoords = projCoords.xy + offsets[i].xy * texelSize;
		float texelDepth = texture(shadowMaps, vec4(texCoords, layer, projCoords.z));
		shadow += texelDepth;
	}

	shadow /= 20.0;
	return shadow;
}

void main()
{
	int entID = int(fs_in.entityID);
	int rInt = int(mod(int(entID / 65025.0), 255));
	int gInt = int(mod(int(entID / 255.0), 255));
	int bInt = int(mod(entID, 255));
	float r = float(rInt) / 255.0;
	float g = float(gInt) / 255.0;
	float b = float(bInt) / 255.0;
	gPicker = vec4(r, g, b, 1.0);
	
	Material mat = u_Materials.materials[int(fs_in.materialSlot)];
	vec3 V = normalize(fs_in.tangentViewPos - fs_in.tangentWorldPos);

	vec2 texCoords = fs_in.textureUV * mat.tilingFactor + mat.texOffset;
	if(bool(mat.isDepthMap))
	{
		texCoords = depthMapUV(texCoords, V, u_Textures[mat.heightTextureSlot], mat.heightFactor);
	}
	else
	{
		texCoords = heightMapUV(texCoords, V, u_Textures[mat.heightTextureSlot], mat.heightFactor);
	}

	if(texCoords.x < -0.01 || texCoords.y < -0.01
		|| texCoords.x > mat.tilingFactor.x + mat.texOffset.x + 0.01 
		|| texCoords.y > mat.tilingFactor.y + mat.texOffset.y + 0.01)
	{
		discard;
	}

	vec4 diffuseColor = texture(u_Textures[mat.albedoTextureSlot], texCoords);
	if(diffuseColor.a == 0.0)
	{
		gDefault = vec4(0.0);
		return;
	}

	if(u_IsLightSource)
	{
		gDefault.rgb = diffuseColor.rgb * mat.color.rgb;
		gDefault.a = diffuseColor.a * mat.color.a;
		return;
	}
	
	float roughness = texture(u_Textures[mat.roughnessTextureSlot], texCoords).r * mat.roughnessFactor;
	float metallic = texture(u_Textures[mat.metallicTextureSlot], texCoords).r * mat.metallicFactor;
	float AO = texture(u_Textures[mat.ambientOccTextureSlot], texCoords).r * mat.ambientOccFactor;
	vec3 N = texture(u_Textures[mat.normalTextureSlot], texCoords).rgb;
	N = N * 2.0 - 1.0;
	
	vec3 Lo = vec3(0.0);
	vec3 F0 = mix(vec3(0.04), diffuseColor.rgb, metallic);
	for(int i = 0; i < MAX_DIR_LIGHTS; i++)
	{
		if(i >= u_DirLights.count)
		{
			break;
		}

		DirectionalLight light = u_DirLights.lights[i];
		vec3 radiance = light.color.rgb;
		vec3 L = fs_in.TBN * light.direction.xyz;
		vec3 H = normalize(V + L);

		float NDF = distGGX(N, H, roughness);
		float G	  = geoSmith(N, V, L, roughness);
		vec3 F	  = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

		float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
		vec3 specular = NDF * G * F / denom;
		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;
		
		float shadow = cascadedShadowFactor(i, N, L);
		Lo += (kD * diffuseColor.rgb / PI + specular) * radiance * max(dot(N, L), 0.0) * shadow;
	}

	int layer = 0;
	for(int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		if(i >= u_PointLights.count)
		{
			break;
		}

		PointLight pointLight = u_PointLights.lights[i];
		vec3 position	= pointLight.positionAndLin.xyz;
		vec3 tangentPos = fs_in.TBN * position;
		vec3 color		= pointLight.colorAndQuad.xyz;
		float linear	= pointLight.positionAndLin.w;
		float quadratic = pointLight.colorAndQuad.w;

		vec3 L = normalize(tangentPos - fs_in.tangentWorldPos);
		vec3 H = normalize(V + L);
		float dist = length(position - fs_in.worldPos);
		float attenuation = 1.0 / (1.0 + linear * dist + quadratic * dist * dist);
		vec3 radiance = color * attenuation;

		float NDF = distGGX(N, H, roughness);
		float G	  = geoSmith(N, V, L, roughness);
		vec3 F	  = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

		float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
		vec3 specular = NDF * G * F / denom;
		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;
		
		vec3 worldDir = fs_in.worldPos - position;
		float maxDot = -1.0;
		int targetDir = 0;
		int localLayer = layer;
		for(int face = 0; face < 6; face++)
		{
			if(face >= pointLight.facesRendered)
			{
				break;
			}

			float d = dot(worldDir, pointLight.renderedDirs[face].xyz);
			maxDot = mix(d, maxDot, float(d < maxDot));
			targetDir = int(mix(face, targetDir, float(d < maxDot)));
			localLayer = int(mix(layer + face, localLayer, float(d < maxDot)));
		}
		
		// float shadow = shadowFactor(u_PointLightShadowmaps, pointLight.lightSpaceMatrices[targetDir], localLayer, N, L);
		Lo += (kD * diffuseColor.rgb / PI + specular) * radiance * max(dot(N, L), 0.0);

		layer += pointLight.facesRendered;
	}
	
	for(int i = 0; i < MAX_SPOTLIGHTS; i++)
	{
		if(i >= u_Spotlights.count)
		{
			break;
		}

		Spotlight spotlight = u_Spotlights.lights[i];
		vec3 position	  = spotlight.positionAndCutoff.xyz;
		vec3 tangentPos	  = fs_in.TBN * position;
		vec3 direction	  = fs_in.TBN * spotlight.directionAndOuterCutoff.xyz;
		vec3 color		  = spotlight.colorAndLin.xyz;
		float cutoff	  = spotlight.positionAndCutoff.w;
		float outerCutoff = spotlight.directionAndOuterCutoff.w;
		float linear	  = spotlight.colorAndLin.w;
		float quadratic	  = spotlight.quadraticTerm.x;
		
		vec3 L = normalize(tangentPos - fs_in.tangentWorldPos);
		float theta = dot(L, normalize(-direction));
		float epsilon = abs(cutoff - outerCutoff) + 0.0001;
		float intensity = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);

		if(theta > cutoff)
		{
			vec3 H = normalize(V + L);
			float dist = length(position - fs_in.worldPos);
			float attenuation = 1.0 / (1.0 + linear * dist + quadratic * dist * dist);
			vec3 radiance = color * attenuation;

			float NDF = distGGX(N, H, roughness);
			float G	  = geoSmith(N, V, L, roughness);
			vec3 F	  = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

			float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
			vec3 specular = NDF * G * F / denom;
			vec3 kS = F;
			vec3 kD = vec3(1.0) - kS;
			kD *= 1.0 - metallic;

			float shadow = shadowFactor(u_SpotlightShadowmaps, spotlight.lightSpaceMatrix, i, N, L);
			Lo += (kD * diffuseColor.rgb / PI + specular) * radiance * max(dot(N, L), 0.0) * intensity * shadow;
		}
	}

	const float MAX_REFL_LOD = 7.0;
	N = transpose(fs_in.TBN) * N;
	V = normalize(fs_in.eyePos - fs_in.worldPos);
	vec3 R = reflect(-V, N);
	vec3 prefilteredColor = textureLod(u_PrefilterMap, R, roughness * MAX_REFL_LOD).rgb;
	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
	vec2 envBRDF = texture(u_BRDF_LUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
	vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

	vec3 kS = F;
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	vec3 irradiance = texture(u_IrradianceMap, N).rgb;
	vec3 diffuse = irradiance * diffuseColor.rgb;
	vec3 ambient = (kD * diffuse + specular) * AO;
	
	gDefault.rgb = (ambient + Lo) * mat.color.rgb;
	gDefault.a = diffuseColor.a * mat.color.a;
}