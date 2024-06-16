#version 430 core

#define MAX_POINT_LIGHTS ${MAX_POINT_LIGHTS}

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

layout(std140, binding = 3) uniform PointLights
{
	PointLight lights[MAX_POINT_LIGHTS];
	int count;
} u_PointLights;

uniform int u_LightID;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gColor;
uniform sampler2D gMaterial;

uniform int u_OffsetsTexSize;
uniform int u_OffsetsFilterSize;
uniform float u_OffsetsRadius;
uniform sampler3D u_OffsetsTexture;
uniform sampler2DArrayShadow u_PointLightShadowmaps;

out vec4 o_Color;

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

float shadowFactor(mat4 lightSpaceMat, int layer, vec3 N, vec3 L, vec3 worldPos)
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
	const vec2 texelSize = 1.0 / textureSize(u_PointLightShadowmaps, 0).xy;

	float depth = 0.0;
	float shadow = 0.0;
	for(int i = 0; i < 4; i++)
	{
		offsetCoord.x = i;
		vec4 offsets = texelFetch(u_OffsetsTexture, offsetCoord, 0) * u_OffsetsRadius;

		sc.xy = projCoords.xy + offsets.rg * texelSize;
		depth = texture(u_PointLightShadowmaps, vec4(sc.xy, layer, projCoords.z));
		shadow += depth;

		sc.xy = projCoords.xy + offsets.ba * texelSize;
		depth = texture(u_PointLightShadowmaps, vec4(sc.xy, layer, projCoords.z));
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
		depth = texture(u_PointLightShadowmaps, vec4(sc.xy, layer, projCoords.z));
		shadow += depth;

		sc.xy = projCoords.xy + offsets.ba * texelSize;
		depth = texture(u_PointLightShadowmaps, vec4(sc.xy, layer, projCoords.z));
		shadow += depth;
	}

	shadow /= float(samplesDiv2) * 2.0;
	return shadow;
}

void main()
{
	vec2 texCoord = gl_FragCoord.xy / u_Camera.screenSize;
	vec3 worldPos = texture(gPosition, texCoord).rgb;
	vec3 N = texture(gNormal, texCoord).rgb;

	vec3 V = normalize(u_Camera.position.xyz - worldPos);

	vec3 materialData = texture(gMaterial, texCoord).rgb;
	float roughness = materialData.r;
	float metallic = materialData.g;
	float ao = materialData.b;

	vec4 diffuseColor = texture(gColor, texCoord);
	vec3 F0 = mix(vec3(0.04), diffuseColor.rgb, metallic);

	PointLight pointLight = u_PointLights.lights[u_LightID];
	vec3 lightPos	= pointLight.positionAndLin.xyz;
	vec3 color		= pointLight.colorAndQuad.xyz;
	float linear	= pointLight.positionAndLin.w;
	float quadratic = pointLight.colorAndQuad.w;

	vec3 L = normalize(lightPos - worldPos);
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

	vec3 worldDir = worldPos - lightPos;
	float maxDot = -1.0;
	int targetDir = 0;
	int layer = u_LightID * 6;
	int localLayer = layer;
	for(int face = 0; face < 6; face++)
	{
		float d = dot(worldDir, pointLight.renderedDirs[face].xyz);
		if(d > maxDot)
		{
			maxDot = d;
			targetDir = face;
			localLayer = layer + face;
		}
	}
		
	float shadow = shadowFactor(pointLight.lightSpaceMatrices[targetDir], localLayer, N, L, worldPos);
	o_Color.rgb = (kD * diffuseColor.rgb / PI + specular) * radiance * max(dot(N, L), 0.0) * shadow;
	o_Color.a = diffuseColor.a;
}