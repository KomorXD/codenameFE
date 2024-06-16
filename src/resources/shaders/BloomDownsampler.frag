#version 430 core

in vec2 textureUV;

uniform sampler2D u_SourceTexture;
uniform vec2 u_SourceResolution;
uniform bool u_FirstMip;
uniform float u_Threshold;

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

out vec4 downsample;

vec3 pow_v3(vec3 v, float p)
{
	return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

vec3 to_sRGB(vec3 v)
{
	return pow_v3(v, 1.0 / u_Camera.gamma);
}

float RGB_ToLuminance(vec3 color)
{
	return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

float KarisAverage(vec3 color)
{
	float lum = RGB_ToLuminance(to_sRGB(color)) * 0.25;
	return 1.0 / (1.0 + lum);
}

vec3 prefilter(vec3 color)
{
	float brightness = max(color.r, max(color.g, color.b));
	float contrib = max(0, brightness - u_Threshold);
	contrib /= brightness + 0.00001;
	return color * contrib;
}

void main()
{
	vec2 srcTexelSize = 1.0 / u_SourceResolution;
	float x = srcTexelSize.x;
	float y = srcTexelSize.y;

	vec3 a = texture(u_SourceTexture, vec2(textureUV.x - 2*x, textureUV.y + 2*y)).rgb;
    vec3 b = texture(u_SourceTexture, vec2(textureUV.x,       textureUV.y + 2*y)).rgb;
    vec3 c = texture(u_SourceTexture, vec2(textureUV.x + 2*x, textureUV.y + 2*y)).rgb;

    vec3 d = texture(u_SourceTexture, vec2(textureUV.x - 2*x, textureUV.y)).rgb;
    vec3 e = texture(u_SourceTexture, vec2(textureUV.x,       textureUV.y)).rgb;
    vec3 f = texture(u_SourceTexture, vec2(textureUV.x + 2*x, textureUV.y)).rgb;

    vec3 g = texture(u_SourceTexture, vec2(textureUV.x - 2*x, textureUV.y - 2*y)).rgb;
    vec3 h = texture(u_SourceTexture, vec2(textureUV.x,       textureUV.y - 2*y)).rgb;
    vec3 i = texture(u_SourceTexture, vec2(textureUV.x + 2*x, textureUV.y - 2*y)).rgb;

    vec3 j = texture(u_SourceTexture, vec2(textureUV.x - x, textureUV.y + y)).rgb;
    vec3 k = texture(u_SourceTexture, vec2(textureUV.x + x, textureUV.y + y)).rgb;
    vec3 l = texture(u_SourceTexture, vec2(textureUV.x - x, textureUV.y - y)).rgb;
    vec3 m = texture(u_SourceTexture, vec2(textureUV.x + x, textureUV.y - y)).rgb;

	if(u_FirstMip)
	{
		vec3 groups[5];

		groups[0] = (a + b + d + e) * (0.125 / 4.0);
		groups[1] = (b + c + e + f) * (0.125 / 4.0);
		groups[2] = (d + e + g + h) * (0.125 / 4.0);
		groups[3] = (e + f + h + i) * (0.125 / 4.0);
		groups[4] = (j + k + l + m) * (0.5 / 4.0);

		groups[0] *= KarisAverage(groups[0]);
		groups[1] *= KarisAverage(groups[1]);
		groups[2] *= KarisAverage(groups[2]);
		groups[3] *= KarisAverage(groups[3]);
		groups[4] *= KarisAverage(groups[4]);

		downsample.rgb = groups[0] + groups[1] + groups[2] + groups[3] + groups[4];
		downsample.rgb = prefilter(downsample.rgb);
	}
	else
	{
		downsample = vec4(e * 0.125, 0.0);
		downsample += vec4((a + c + g + i) * 0.03125, 0.0);
		downsample += vec4((b + d + f + h) * 0.0625, 0.0);
		downsample += vec4((j + k + l + m) * 0.125, 0.0);
	}

	downsample = max(downsample, 0.0001);
	downsample.a = 1.0;
}