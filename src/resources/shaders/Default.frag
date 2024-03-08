#version 430 core

struct DirectionalLight
{
	vec4 direction;
	vec4 color;
};

struct PointLight
{
	vec3 position;
	float linearTerm;

	vec3 color;
	float quadraticTerm;
};

struct SpotLight
{
	vec3 position;
	float cutOff;

	vec3 direction;
	float outerCutOff;

	vec3 color;
	float linearTerm;
	
	float quadraticTerm;
};

in VS_OUT
{
	vec3 worldPos;
	mat3 TBN;
	vec4 color;
	vec2 textureUV;
	flat float textureSlot;
	flat float normalTextureSlot;
} fs_in;

layout(std430, binding = 0) readonly buffer DirectionalLights
{
	DirectionalLight lights[128];
	int count;
} dirLights;

layout(std430, binding = 1) readonly buffer PointLights
{
	PointLight lights[128];
	int count;
} pointLights;

layout(std430, binding = 2) readonly buffer SpotLights
{
	SpotLight lights[128];
	int count;
} spotLights;

uniform vec3 u_ViewPos = vec3(0.0);
uniform bool u_IsLightSource = false;
uniform float u_AmbientStrength = 0.1;
uniform sampler2D u_Textures[24];

out vec4 fragColor;

int parseSlot(float inSlot)
{
	int slot = -1;
	switch(int(inSlot))
	{
		case  0: slot = 0;  break;
		case  1: slot = 1;  break;
		case  2: slot = 2;  break;
		case  3: slot = 3;  break;
		case  4: slot = 4;  break;
		case  5: slot = 5;  break;
		case  6: slot = 6;  break;
		case  7: slot = 7;  break;
		case  8: slot = 8;  break;
		case  9: slot = 9;  break;
		case 10: slot = 10; break;
		case 11: slot = 11; break;
		case 12: slot = 12; break;
		case 13: slot = 13; break;
		case 14: slot = 14; break;
		case 15: slot = 15; break;
		case 16: slot = 16; break;
		case 17: slot = 17; break;
		case 18: slot = 18; break;
		case 19: slot = 19; break;
		case 20: slot = 20; break;
		case 21: slot = 21; break;
		case 22: slot = 22; break;
		case 23: slot = 23; break;
	}

	return slot;
}

void main()
{
	int albedoSlot = parseSlot(fs_in.textureSlot);
	int normalSlot = parseSlot(fs_in.normalTextureSlot);

	vec4 color = texture(u_Textures[albedoSlot], fs_in.textureUV) * fs_in.color;
	vec3 normal = texture(u_Textures[normalSlot], fs_in.textureUV).rgb;
	normal = normal * 2.0 - 1.0;
	normal = normalize(fs_in.TBN * normal);
	
	if(color.a == 0.0)
	{
		discard;
		return;
	}

	if(u_IsLightSource)
	{
		fragColor = vec4(pow(color.rgb, vec3(1.0 / 2.2)), color.a);
		return;
	}

	vec3 totalDirectional = vec3(0.0);
	for(int i = 0; i < dirLights.count; i++)
	{
		DirectionalLight light = dirLights.lights[i];
		totalDirectional += max(dot(normal, -light.direction.xyz), 0.0) * light.color.rgb;
	}

	vec3 totalDiffuse = vec3(0.0);
	vec3 totalSpecular = vec3(0.0);
	for(int i = 0; i < pointLights.count; i++)
	{
		PointLight pointLight = pointLights.lights[i];
		
		vec3 lightDir = normalize(pointLight.position - fs_in.worldPos);
		float fragToLightDist = length(pointLight.position - fs_in.worldPos);
		float attenuation = 1.0 / (1.0 + pointLight.linearTerm * fragToLightDist + pointLight.quadraticTerm * fragToLightDist * fragToLightDist);
		
		vec3 viewDir = normalize(u_ViewPos - fs_in.worldPos);
		vec3 halfway = normalize(lightDir + viewDir);

		totalDiffuse += max(dot(normal, lightDir), 0.0) * pointLight.color * attenuation * step(0.0, dot(normal, lightDir));
		totalSpecular += pow(max(dot(normal, halfway), 0.0), 32.0) * pointLight.color * attenuation * step(0.0, dot(normal, lightDir));
	}

	for(int i = 0; i < spotLights.count; i++)
	{
		SpotLight spotLight = spotLights.lights[i];

		vec3 lightDir = normalize(spotLight.position - fs_in.worldPos);
		float theta = dot(lightDir, normalize(-spotLight.direction));
		float epsilon = abs(spotLight.cutOff - spotLight.outerCutOff) + 0.0001;
		float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);

		if(theta > spotLight.cutOff)
		{
			float fragToLightDist = length(spotLight.position - fs_in.worldPos);
			float attenuation = 1.0 / (1.0 + spotLight.linearTerm * fragToLightDist + spotLight.quadraticTerm * fragToLightDist * fragToLightDist);
		
			vec3 viewDir = normalize(u_ViewPos - fs_in.worldPos);
			vec3 halfway = normalize(lightDir + viewDir);

			totalDiffuse += max(dot(normal, lightDir), 0.0) * spotLight.color * attenuation * step(0.0, dot(normal, lightDir)) * intensity;
			totalSpecular += pow(max(dot(normal, halfway), 0.0), 32.0) * spotLight.color * attenuation * step(0.0, dot(normal, lightDir)) * intensity;
		}
	}

	fragColor.rgb = (u_AmbientStrength + totalDirectional + totalDiffuse + totalSpecular) * color.rgb;
	fragColor.rgb = pow(fragColor.rgb, vec3(1.0 / 2.2));
	fragColor.a = color.a;
}