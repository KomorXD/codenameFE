#version 430 core

#define MAX_DIR_LIGHTS ${MAX_DIR_LIGHTS}
#define MAX_SPOTLIGHTS ${MAX_SPOTLIGHTS}

struct DirectionalLight
{
	mat4 cascadeLightMatrices[${CASCADES_COUNT}];
	vec4 direction;
	vec4 color;
};

struct Spotlight
{
	mat4 lightSpaceMatrix;
	vec4 positionAndCutoff;
	vec4 directionAndOuterCutoff;
	vec4 colorAndLin;
	vec4 quadraticTerm;
};

layout (location = 0) out vec4 o_Color;
layout (location = 1) out vec4 o_Picker;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColor;
uniform sampler2D gMaterial;
uniform sampler2D gLights;
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_PrefilterMap;
uniform sampler2D u_BRDF_LUT;

uniform float u_CascadeDistances[${CASCADES_COUNT}];
uniform sampler2DArrayShadow u_DirLightCSM;
uniform sampler2DArrayShadow u_SpotlightShadowmaps;

uniform int u_OffsetsTexSize;
uniform int u_OffsetsFilterSize;
uniform float u_OffsetsRadius;
uniform sampler3D u_OffsetsTexture;

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

layout(std140, binding = 2) uniform DirectionalLights
{
	DirectionalLight lights[MAX_DIR_LIGHTS];
	int count;
} u_DirLights;

layout(std140, binding = 4) uniform Spotlights
{
	Spotlight lights[MAX_SPOTLIGHTS];
	int count;
} u_Spotlights;

in VS_OUT
{
	vec2 textureUV;
} fs_in;

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

float cascadedShadowFactor(int dirLightIdx, vec3 N, vec3 L, vec3 worldPos)
{
	vec4 fragPosViewSpace = u_Camera.view * vec4(worldPos, 1.0);
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

	vec4 fragPosLightSpace = u_DirLights.lights[dirLightIdx].cascadeLightMatrices[layer] * vec4(worldPos, 1.0);
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

float shadowFactor(sampler2DArrayShadow shadowMaps, mat4 lightSpaceMat, int layer, vec3 N, vec3 L, vec3 worldPos)
{
	vec4 fragPosLightSpace = lightSpaceMat * vec4(worldPos, 1.0);
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

void main()
{
	int entID = int(texture(gPosition, fs_in.textureUV).a);
	if(entID == 0)
	{
		discard;
	}

	int rInt = int(mod(int(entID / 65025.0), 255));
	int gInt = int(mod(int(entID / 255.0), 255));
	int bInt = int(mod(entID, 255));
	float r = float(rInt) / 255.0;
	float g = float(gInt) / 255.0;
	float b = float(bInt) / 255.0;
	o_Picker = vec4(r, g, b, 1.0);

	vec3 worldPos = texture(gPosition, fs_in.textureUV).rgb;
	vec3 N = texture(gNormal, fs_in.textureUV).rgb;

	vec3 V = normalize(u_Camera.position.xyz - worldPos);

	vec3 materialData = texture(gMaterial, fs_in.textureUV).rgb;
	float roughness = materialData.r;
	float metallic = materialData.g;
	float ao = materialData.b;

	vec3 Lo = vec3(0.0);
	vec4 diffuseColor = texture(gColor, fs_in.textureUV);
	vec3 F0 = mix(vec3(0.04), diffuseColor.rgb, metallic);
	for(int i = 0; i < MAX_DIR_LIGHTS; i++)
	{
		if(i >= u_DirLights.count)
		{
			break;
		}

		DirectionalLight light = u_DirLights.lights[i];
		vec3 radiance = light.color.rgb;
		vec3 L = light.direction.xyz;
		vec3 H = normalize(V + L);

		float NDF = distGGX(N, H, roughness);
		float G	  = geoSmith(N, V, L, roughness);
		vec3 F	  = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

		float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
		vec3 specular = NDF * G * F / denom;
		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;

		float shadow = cascadedShadowFactor(i, N, L, worldPos);
		Lo += (kD * diffuseColor.rgb / PI + specular) * radiance * max(dot(N, L), 0.0) * shadow;
	}

	for(int i = 0; i < MAX_SPOTLIGHTS; i++)
	{
		if(i >= u_Spotlights.count)
		{
			break;
		}

		Spotlight spotlight = u_Spotlights.lights[i];
		vec3 lightPos	  = spotlight.positionAndCutoff.xyz;
		vec3 direction	  = spotlight.directionAndOuterCutoff.xyz;
		vec3 color		  = spotlight.colorAndLin.xyz;
		float cutoff	  = spotlight.positionAndCutoff.w;
		float outerCutoff = spotlight.directionAndOuterCutoff.w;
		float linear	  = spotlight.colorAndLin.w;
		float quadratic	  = spotlight.quadraticTerm.x;
		
		vec3 L = normalize(lightPos - worldPos);
		float theta = dot(L, normalize(-direction));
		float epsilon = abs(cutoff - outerCutoff) + 0.0001;
		float intensity = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);

		if(theta > cutoff)
		{
			vec3 H = normalize(V + L);
			float dist = length(lightPos - worldPos);
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
			
			float shadow = shadowFactor(u_SpotlightShadowmaps, spotlight.lightSpaceMatrix, i, N, L, worldPos);
			Lo += (kD * diffuseColor.rgb / PI + specular) * radiance * max(dot(N, L), 0.0) * intensity * shadow;
		}
	}
	
	const float MAX_REFL_LOD = 7.0;
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
	vec3 ambient = (kD * diffuse + specular) * ao;

	Lo += texture(gLights, gl_FragCoord.xy / u_Camera.screenSize).rgb;
	o_Color.rgb = ambient + Lo;
	o_Color.a = diffuseColor.a;
}