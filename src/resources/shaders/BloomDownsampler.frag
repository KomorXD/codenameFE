#version 430 core

in vec2 textureUV;

uniform sampler2D u_SourceTexture;
uniform vec2 u_SourceResolution;

out vec4 downsample;

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

    downsample = vec4(e * 0.125, 0.0);
    downsample += vec4((a + c + g + i) * 0.03125, 0.0);
    downsample += vec4((b + d + f + h) * 0.0625, 0.0);
    downsample += vec4((j + k + l + m) * 0.125, 0.0);
	downsample = max(downsample, 0.0001);
	downsample.a = 1.0;
}