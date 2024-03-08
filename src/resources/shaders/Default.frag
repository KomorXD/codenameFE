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
	mat3 TBN;
	vec4 color;
	vec2 textureUV;
	flat float textureSlot;
	flat float normalTextureSlot;
} fs_in;

uniform vec3 u_ViewPos = vec3(0.0);
uniform bool u_IsLightSource = false;
uniform float u_AmbientStrength = 0.1;

uniform DirectionalLight u_DirLights[4];
uniform int u_DirLightsCount = 0;

uniform PointLight u_PointLights[8];
uniform int u_PointLightsCount = 0;

uniform SpotLight u_SpotLights[8];
uniform int u_SpotLightsCount = 0;

uniform sampler2D u_Textures[24];

out vec4 fragColor;

void main()
{
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

	int normalSlot = -1;
	switch(int(fs_in.normalTextureSlot))
	{
		case  0: normalSlot = 0;  break;
		case  1: normalSlot = 1;  break;
		case  2: normalSlot = 2;  break;
		case  3: normalSlot = 3;  break;
		case  4: normalSlot = 4;  break;
		case  5: normalSlot = 5;  break;
		case  6: normalSlot = 6;  break;
		case  7: normalSlot = 7;  break;
		case  8: normalSlot = 8;  break;
		case  9: normalSlot = 9;  break;
		case 10: normalSlot = 10; break;
		case 11: normalSlot = 11; break;
		case 12: normalSlot = 12; break;
		case 13: normalSlot = 13; break;
		case 14: normalSlot = 14; break;
		case 15: normalSlot = 15; break;
		case 16: normalSlot = 16; break;
		case 17: normalSlot = 17; break;
		case 18: normalSlot = 18; break;
		case 19: normalSlot = 19; break;
		case 20: normalSlot = 20; break;
		case 21: normalSlot = 21; break;
		case 22: normalSlot = 22; break;
		case 23: normalSlot = 23; break;
	}

	vec4 color = texture(u_Textures[slot], fs_in.textureUV) * fs_in.color;
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
	for(int i = 0; i < u_DirLightsCount; i++)
	{
		totalDirectional += max(dot(normal, -u_DirLights[i].direction), 0.0) * u_DirLights[i].color;
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

		totalDiffuse += max(dot(normal, lightDir), 0.0) * pointLight.color * attenuation * step(0.0, dot(normal, lightDir));
		totalSpecular += pow(max(dot(normal, halfway), 0.0), 32.0) * pointLight.color * attenuation * step(0.0, dot(normal, lightDir));
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

			totalDiffuse += max(dot(normal, lightDir), 0.0) * spotLight.color * attenuation * step(0.0, dot(normal, lightDir)) * intensity;
			totalSpecular += pow(max(dot(normal, halfway), 0.0), 32.0) * spotLight.color * attenuation * step(0.0, dot(normal, lightDir)) * intensity;
		}
	}

	fragColor.rgb = (u_AmbientStrength + totalDirectional + totalDiffuse + totalSpecular) * color.rgb;
	fragColor.rgb = pow(fragColor.rgb, vec3(1.0 / 2.2));
	fragColor.a = color.a;
}