#version 430 core

layout(location = 0) out vec4 gDefault;
layout(location = 1) out vec4 gPicker;

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

struct Spotlight
{
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
	float exposure;
	float gamma;
	float near;
	float far;
} u_Camera;

layout(std140, binding = 1) uniform Materials
{
	Material materials[128];
} u_Materials;

layout(std140, binding = 2) uniform DirectionalLights
{
	DirectionalLight lights[128];
	int count;
} u_DirLights;

layout(std140, binding = 3) uniform PointLights
{
	PointLight lights[128];
	int count;
} u_PointLights;

layout(std140, binding = 4) uniform Spotlights
{
	Spotlight lights[128];
	int count;
} u_Spotlights;

uniform bool u_IsLightSource = false;
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_PrefilterMap;
uniform sampler2D u_BRDF_LUT;
uniform sampler2D u_Textures[64];

uniform samplerCubeArray u_PointLightShadowmaps;

uniform sampler2DArray u_SpotlightShadowmaps;
uniform mat4 u_SpotlightMatrices[16];

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
	if(heightScale <= 0.0)
	{
		return texCoords;
	}

	const float minLayers = 8.0;
	const float maxLayers = 64.0;
	float layers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));

	float layerDepth = 1.0 / layers;
	float currentDepth = 0.0;
	vec2 p = vec2(viewDir.x, -viewDir.y) * heightScale;
	vec2 deltaCoords = p / layers;

	vec2 currentCoords = texCoords;
	float depthMapValue = texture(depthMap, currentCoords).r;
	while(currentDepth < depthMapValue)
	{
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
	if(heightScale <= 0.0)
	{
		return texCoords;
	}

	const float minLayers = 8.0;
	const float maxLayers = 64.0;
	float layers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));

	float layerDepth = 1.0 / layers;
	float currentDepth = 0.0;
	vec2 p = vec2(viewDir.x, -viewDir.y) * heightScale;
	vec2 deltaCoords = p / layers;

	vec2 currentCoords = texCoords;
	float depthMapValue = 1.0 - texture(depthMap, currentCoords).r;
	while(currentDepth < depthMapValue)
	{
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

float pointLightShadowFactor(int lightIdx, vec3 lightPos)
{
	vec3 fragToLight = fs_in.worldPos - lightPos;
	float currentDepth = dot(fragToLight, fragToLight);
	float shadow = 0.0;
	float bias = 0.15;
	float viewDistance = length(fs_in.worldPos - u_Camera.position.xyz);
	float radius = (1.0 + (viewDistance / u_Camera.far)) / 25.0;
	int samples = offsets.length();
	for(int i = 0; i < samples; i++)
	{
		float closestDepth = texture(u_PointLightShadowmaps, vec4(fragToLight + offsets[i] * radius, lightIdx)).r;
		closestDepth *= 1000.0;

		if(currentDepth - bias > closestDepth)
		{
			shadow += 1.0;
		}
	}

	shadow /= float(samples);
	return 1.0 - shadow;
}

float shadowFactor(int lightIdx)
{
	vec4 fragPosLightSpace = u_SpotlightMatrices[lightIdx] * vec4(fs_in.worldPos, 1.0);
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;

	float currentDepth = projCoords.z;
	if(currentDepth > 1.0)
	{
		return 0.0;
	}

	float shadow = 0.0;
	float bias = 0.00005;
	vec2 texelSize = 1.0 / textureSize(u_SpotlightShadowmaps, 0).xy;
	int samples = offsets.length();
	for(int i = 0; i < samples; i++)
	{
		float depth = texture(u_SpotlightShadowmaps, vec3(projCoords.xy + offsets[i].xy * texelSize, lightIdx)).r;
		if(currentDepth - bias > depth)
		{
			shadow += 1.0;
		}
	}
	shadow /= float(samples);
	return 1.0 - shadow;
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
	for(int i = 0; i < u_DirLights.count; i++)
	{
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
		
		Lo += (kD * diffuseColor.rgb / PI + specular) * radiance * max(dot(N, L), 0.0);
	}

	for(int i = 0; i < u_PointLights.count; i++)
	{
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

		float shadow = pointLightShadowFactor(i, position);
		Lo += (kD * diffuseColor.rgb / PI + specular) * radiance * max(dot(N, L), 0.0) * shadow;
	}
	
	vec3 totalDiffuse = vec3(0.0);
	vec3 totalSpecular = vec3(0.0);
	for(int i = 0; i < u_Spotlights.count; i++)
	{
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

			float shadow = shadowFactor(i);
			Lo += (kD * diffuseColor.rgb / PI + specular) * radiance * max(dot(N, L), 0.0) * intensity * shadow;
		}
	}

	const float MAX_REFL_LOD = 4.0;
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