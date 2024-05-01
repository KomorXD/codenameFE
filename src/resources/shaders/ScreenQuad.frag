#version 430 core

layout(location = 0) out vec4 gOut;
  
in vec2 textureUV;

uniform sampler2D u_ScreenTexture;
uniform sampler2D u_BloomTexture;
uniform float u_BloomStrength;

layout (std140, binding = 0) uniform Camera
{
	mat4 projection;
	mat4 view;
	vec4 position;
	float exposure;
	float gamma;
	float near;
	float far;
} u_Camera;

void main()
{
	vec4 hdr = texture(u_ScreenTexture, textureUV);
	vec3 bloom = texture(u_BloomTexture, textureUV).rgb;
	// vec3 result = mix(hdr.rgb, bloom, u_BloomStrength);
	vec3 result = hdr.rgb + bloom * u_BloomStrength;
	vec3 mapped = vec3(1.0) - exp(-result * u_Camera.exposure);
	mapped = pow(mapped, vec3(1.0 / u_Camera.gamma));
    gOut = vec4(mapped, hdr.a);
}