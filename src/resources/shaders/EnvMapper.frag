#version 430 core

layout (location = 0) out vec4 gDefault;
layout (location = 1) out vec4 gIrradiance;

in vec3 localPos;

uniform sampler2D u_EquirectangularEnvMap;

const vec2 INV_ATAN = vec2(0.1591, 0.3183);
vec2 sampleEquiMap(vec3 N)
{
	vec2 uv = vec2(atan(N.z, N.x), asin(N.y));
	uv *= INV_ATAN;
	uv += 0.5;

	return uv;
}

void main()
{
	vec3 N = normalize(localPos);
	vec2 uv = sampleEquiMap(N);
	vec3 color = texture(u_EquirectangularEnvMap, uv).rgb;

	gDefault = vec4(color, 1.0);

	vec3 irradiance = vec3(0.0);
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, N));
	up = normalize(cross(N, right));

	const float PI = 3.14159265359;
	float dPhi = 0.05;
	float dTheta = 0.05;
	int samples = 0;
	for(float phi = 0.0; phi < 2.0 * PI; phi += dPhi)
	{
		for(float theta = 0.0; theta < 0.5 * PI; theta += dTheta)
		{
			vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
			vec2 sampleUV = sampleEquiMap(sampleVec);

			irradiance += texture(u_EquirectangularEnvMap, sampleUV).rgb * cos(theta) * sin(theta);
			samples++;
		}
	}

	irradiance = PI * irradiance * (1.0 / float(samples));
	gIrradiance = vec4(irradiance, 1.0);
}