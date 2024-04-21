#version 430 core

out vec4 fragColor;

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
	vec2 uv = sampleEquiMap(normalize(localPos));
	vec3 color = texture(u_EquirectangularEnvMap, uv).rgb;

	fragColor = vec4(color, 1.0);
}