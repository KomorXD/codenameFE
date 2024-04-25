#version 430 core

in vec2 textureUV;

uniform sampler2D u_SourceTexture;
uniform float u_FilterRadius;

out vec3 upsample;

void main()
{
    float x = u_FilterRadius;
    float y = u_FilterRadius;

    vec3 a = texture(u_SourceTexture, vec2(textureUV.x - x, textureUV.y + y)).rgb;
    vec3 b = texture(u_SourceTexture, vec2(textureUV.x,     textureUV.y + y)).rgb;
    vec3 c = texture(u_SourceTexture, vec2(textureUV.x + x, textureUV.y + y)).rgb;

    vec3 d = texture(u_SourceTexture, vec2(textureUV.x - x, textureUV.y)).rgb;
    vec3 e = texture(u_SourceTexture, vec2(textureUV.x,     textureUV.y)).rgb;
    vec3 f = texture(u_SourceTexture, vec2(textureUV.x + x, textureUV.y)).rgb;

    vec3 g = texture(u_SourceTexture, vec2(textureUV.x - x, textureUV.y - y)).rgb;
    vec3 h = texture(u_SourceTexture, vec2(textureUV.x,     textureUV.y - y)).rgb;
    vec3 i = texture(u_SourceTexture, vec2(textureUV.x + x, textureUV.y - y)).rgb;

    upsample = e * 4.0;
    upsample += (b + d + f + h) * 2.0;
    upsample += a + c + g + i;
    upsample *= 1.0 / 16.0;
}