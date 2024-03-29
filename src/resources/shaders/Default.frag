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

in VS_OUT
{
	vec3 worldPos;
	mat3 TBN;
	vec4 color;
	vec2 textureUV;
	flat float textureSlot;
	flat float normalTextureSlot;
	flat float entityID;
} fs_in;

layout(std140, binding = 1) uniform Lights
{
	DirectionalLight dirLights[128];
	PointLight pointLights[128];
	SpotLight spotLights[128];
	
	int dirLightsCount;
	int pointLightsCount;
	int spotLightsCount;
} lights;

uniform vec3 u_ViewPos = vec3(0.0);
uniform bool u_IsLightSource = false;
uniform float u_AmbientStrength = 0.1;
uniform sampler2D u_Textures[24];

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

	int albedoSlot = int(fs_in.textureSlot);
	int normalSlot = int(fs_in.normalTextureSlot);
	vec4 diffuseColor = texture(u_Textures[albedoSlot], fs_in.textureUV) * fs_in.color;
	vec3 normal = texture(u_Textures[normalSlot], fs_in.textureUV).rgb;
	normal = normal * 2.0 - 1.0;
	normal = normalize(fs_in.TBN * normal);
	
	if(diffuseColor.a == 0.0)
	{
		return;
	}

	if(u_IsLightSource)
	{
		gDefault = diffuseColor;
		return;
	}

	vec3 totalDirectional = vec3(0.0);
	for(int i = 0; i < lights.dirLightsCount; i++)
	{
		DirectionalLight light = lights.dirLights[i];
		totalDirectional += max(dot(normal, -light.direction.xyz), 0.0) * light.color.rgb;
	}

	vec3 totalDiffuse = vec3(0.0);
	vec3 totalSpecular = vec3(0.0);
	for(int i = 0; i < lights.pointLightsCount; i++)
	{
		PointLight pointLight = lights.pointLights[i];
		vec3 position	= pointLight.positionAndLin.xyz;
		vec3 color		= pointLight.colorAndQuad.xyz;
		float linear	= pointLight.positionAndLin.w;
		float quadratic = pointLight.colorAndQuad.w;
		
		vec3 lightDir = normalize(position - fs_in.worldPos);
		float fragToLightDist = length(position - fs_in.worldPos);
		float attenuation = 1.0 / (1.0 + linear * fragToLightDist + quadratic * fragToLightDist * fragToLightDist);
		
		vec3 viewDir = normalize(u_ViewPos - fs_in.worldPos);
		vec3 halfway = normalize(lightDir + viewDir);

		totalDiffuse += max(dot(normal, lightDir), 0.0) * color * attenuation * step(0.0, dot(normal, lightDir));
		totalSpecular += pow(max(dot(normal, halfway), 0.0), 32.0) * color * attenuation * step(0.0, dot(normal, lightDir));
	}

	for(int i = 0; i < lights.spotLightsCount; i++)
	{
		SpotLight spotLight = lights.spotLights[i];
		vec3 position	  = spotLight.positionAndCutoff.xyz;
		vec3 direction	  = spotLight.directionAndOuterCutoff.xyz;
		vec3 color		  = spotLight.colorAndLin.xyz;
		float cutoff	  = spotLight.positionAndCutoff.w;
		float outerCutoff = spotLight.directionAndOuterCutoff.w;
		float linear	  = spotLight.colorAndLin.w;
		float quadratic	  = spotLight.quadraticTerm.x;

		vec3 lightDir = normalize(position - fs_in.worldPos);
		float theta = dot(lightDir, normalize(-direction));
		float epsilon = abs(cutoff - outerCutoff) + 0.0001;
		float intensity = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);

		if(theta > cutoff)
		{
			float fragToLightDist = length(position - fs_in.worldPos);
			float attenuation = 1.0 / (1.0 + linear * fragToLightDist + quadratic * fragToLightDist * fragToLightDist);
		
			vec3 viewDir = normalize(u_ViewPos - fs_in.worldPos);
			vec3 halfway = normalize(lightDir + viewDir);

			totalDiffuse += max(dot(normal, lightDir), 0.0) * color * attenuation * step(0.0, dot(normal, lightDir)) * intensity;
			totalSpecular += pow(max(dot(normal, halfway), 0.0), 32.0) * color * attenuation * step(0.0, dot(normal, lightDir)) * intensity;
		}
	}

	gDefault.rgb = (u_AmbientStrength + totalDirectional + totalDiffuse + totalSpecular) * diffuseColor.rgb;
	gDefault.a = diffuseColor.a;
}