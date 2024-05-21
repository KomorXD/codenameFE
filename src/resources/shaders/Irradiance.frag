#version 430 core

out vec4 outIrradiance;

in vec3 localPos;

uniform samplerCube u_EnvMap;

void main()
{
	vec3 N = normalize(localPos);
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, N));
	up = normalize(cross(N, right));
	
	const float PI = 3.14159265359;
	vec3 irradiance = vec3(0.0);
	float dPhi = 0.05;
	float dTheta = 0.05;
	float samples = (2.0 * PI / dPhi) * (0.5 * PI / dTheta);
	for(float phi = 0.0; phi < 2.0 * PI; phi += dPhi)
	{
		for(float theta = 0.0; theta < 0.5 * PI; theta += dTheta)
		{
			vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

			irradiance += min(texture(u_EnvMap, sampleVec).rgb * cos(theta) * sin(theta), 4.0) / samples;
		}
	}

	outIrradiance.rgb = PI * irradiance;
	outIrradiance.a = 1.0;
}