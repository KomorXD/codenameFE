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

struct SpotLight
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
	mat3 TBN;
	vec3 tangentWorldPos;
	vec3 tangentViewPos;
	vec2 textureUV;
	flat float materialSlot;
	flat float entityID;
} fs_in;

layout(std140, binding = 2) uniform Lights
{
	DirectionalLight dirLights[128];
	PointLight pointLights[128];
	SpotLight spotLights[128];
	
	int dirLightsCount;
	int pointLightsCount;
	int spotLightsCount;
} lights;

layout(std140, binding = 3) uniform Materials
{
	Material materialsData[128];
} materials;

uniform bool u_IsLightSource = false;
uniform samplerCube u_IrradianceMap;
uniform sampler2D u_Textures[24];

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
	float a	  = roughness = roughness;
	float a2  = a * a;
	float NH  = max(dot(N, H), 0.0);
	float NH2 = NH * NH;
	float denom = (NH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return a2 / denom;
}

float geoSchlickGGX(float NV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
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
	
	Material mat = materials.materialsData[int(fs_in.materialSlot)];
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

	if(texCoords.x < -0.1 || texCoords.y < -0.1
		|| texCoords.x > mat.tilingFactor.x + mat.texOffset.x + 0.1 
		|| texCoords.y > mat.tilingFactor.y + mat.texOffset.y + 0.1)
	{
		discard;
	}

	vec4 diffuseColor = texture(u_Textures[mat.albedoTextureSlot], texCoords) * mat.color;
	if(diffuseColor.a == 0.0)
	{
		gDefault = vec4(0.0);
		return;
	}

	if(u_IsLightSource)
	{
		gDefault = diffuseColor;
		return;
	}
	
	float roughness = texture(u_Textures[mat.roughnessTextureSlot], texCoords).r * mat.roughnessFactor;
	float metallic = texture(u_Textures[mat.metallicTextureSlot], texCoords).r * mat.metallicFactor;
	float AO = texture(u_Textures[mat.ambientOccTextureSlot], texCoords).r * mat.ambientOccFactor;
	vec3 N = texture(u_Textures[mat.normalTextureSlot], texCoords).rgb;
	N = N * 2.0 - 1.0;
	
	vec3 Lo = vec3(0.0);
	vec3 F0 = mix(vec3(0.04), diffuseColor.rgb, metallic);
	for(int i = 0; i < lights.dirLightsCount; i++)
	{
		DirectionalLight light = lights.dirLights[i];
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

	for(int i = 0; i < lights.pointLightsCount; i++)
	{
		PointLight pointLight = lights.pointLights[i];
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
		
		Lo += (kD * diffuseColor.rgb / PI + specular) * radiance * max(dot(N, L), 0.0);
	}
	
	vec3 totalDiffuse = vec3(0.0);
	vec3 totalSpecular = vec3(0.0);
	for(int i = 0; i < lights.spotLightsCount; i++)
	{
		SpotLight spotLight = lights.spotLights[i];
		vec3 position	  = spotLight.positionAndCutoff.xyz;
		vec3 tangentPos	  = fs_in.TBN * position;
		vec3 direction	  = spotLight.directionAndOuterCutoff.xyz;
		vec3 color		  = spotLight.colorAndLin.xyz;
		float cutoff	  = spotLight.positionAndCutoff.w;
		float outerCutoff = spotLight.directionAndOuterCutoff.w;
		float linear	  = spotLight.colorAndLin.w;
		float quadratic	  = spotLight.quadraticTerm.x;
		
		vec3 L = normalize(position - fs_in.worldPos);
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
		
			Lo += (kD * diffuseColor.rgb / PI + specular) * radiance * max(dot(N, L), 0.0) * intensity;
		}
	}

	vec3 kS = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, mat.roughnessFactor);
	vec3 kD = 1.0 - kS;
	vec3 irradiance = texture(u_IrradianceMap, N).rgb;
	vec3 diffuse = irradiance * diffuseColor.rgb;
	vec3 ambient = kD * diffuse * AO;
	
	gDefault.rgb = ambient + Lo;
	gDefault.a = diffuseColor.a;
}