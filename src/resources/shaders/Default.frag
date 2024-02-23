#version 430 core

struct DirectionalLight
{
	vec3 direction;
	vec3 color;
};

struct PointLight
{
	vec3 position;
	vec3 color;
	float linearTerm;
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
	vec3 normal;
	vec4 color;
	vec2 textureUV;
	flat float textureSlot;
} fs_in;

uniform vec3 u_ViewPos = vec3(0.0);
uniform float u_AmbientStrength = 0.1;

uniform DirectionalLight u_DirLights[4];
uniform int u_DirLightsCount = 0;

uniform PointLight u_PointLights[8];
uniform int u_PointLightsCount = 0;

uniform SpotLight u_SpotLights[8];
uniform int u_SpotLightsCount;

uniform sampler2D u_Textures[24];

out vec4 fragColor;

void main()
{
	vec3 totalDirectional = vec3(0.0);
	for(int i = 0; i < u_DirLightsCount; i++)
	{
		totalDirectional += max(dot(fs_in.normal, -u_DirLights[i].direction), 0.0) * u_DirLights[i].color;
	}

	vec3 totalDiffuse = vec3(0.0);
	vec3 totalSpecular = vec3(0.0);
	for(int i = 0; i < u_PointLightsCount; i++)
	{
		PointLight pointLight = u_PointLights[i];
		
		vec3 lightDir = normalize(pointLight.position - fs_in.worldPos);
		float fragToLightDist = length(pointLight.position - fs_in.worldPos);
		float attenuation = 1.0 / (1.0 + pointLight.linearTerm * fragToLightDist + pointLight.quadraticTerm * fragToLightDist * fragToLightDist);
		
		vec3 viewDir = normalize(u_ViewPos - fs_in.worldPos);
		vec3 halfway = normalize(lightDir + viewDir);

		totalDiffuse += max(dot(fs_in.normal, lightDir), 0.0) * pointLight.color * attenuation * step(0.0, dot(fs_in.normal, lightDir));
		totalSpecular += pow(max(dot(fs_in.normal, halfway), 0.0), 32.0) * pointLight.color * attenuation * step(0.0, dot(fs_in.normal, lightDir));
	}

	for(int i = 0; i < u_SpotLightsCount; i++)
	{
		SpotLight spotLight = u_SpotLights[i];

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

			totalDiffuse += max(dot(fs_in.normal, lightDir), 0.0) * spotLight.color * attenuation * step(0.0, dot(fs_in.normal, lightDir)) * intensity;
			totalSpecular += pow(max(dot(fs_in.normal, halfway), 0.0), 32.0) * spotLight.color * attenuation * step(0.0, dot(fs_in.normal, lightDir)) * intensity;
		}
	}

	int slot = -1;
	switch(int(fs_in.textureSlot))
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

	vec4 color = (slot < 0 ? fs_in.color : texture(u_Textures[slot], fs_in.textureUV));
	fragColor.rgb = (u_AmbientStrength + totalDirectional + totalDiffuse + totalSpecular) * color.rgb;
	fragColor.rgb = pow(fragColor.rgb, vec3(1.0 / 2.2));
	fragColor.a = color.a;
}