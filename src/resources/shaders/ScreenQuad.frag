#version 430 core

layout(location = 0) out vec4 gOut;
  
in vec2 textureUV;

uniform sampler2D u_ScreenTexture;
uniform bool u_BlurEnabled = false;
uniform float u_Exposure = 1.0;

const float offset = 1.0 / 300.0;
vec2 offsets[9] = vec2[](
    vec2(-offset,  offset), // top-left
    vec2( 0.0,     offset), // top-center
    vec2( offset,  offset), // top-right
    vec2(-offset,  0.0),    // center-left
    vec2( 0.0,     0.0),    // center-center
    vec2( offset,  0.0),    // center-right
    vec2(-offset, -offset), // bottom-left
    vec2( 0.0,    -offset), // bottom-center
    vec2( offset, -offset)  // bottom-right    
);
float kernel[9] = float[](
    1.0 / 16, 2.0 / 16, 1.0 / 16,
    2.0 / 16, 4.0 / 16, 2.0 / 16,
    1.0 / 16, 2.0 / 16, 1.0 / 16
);

void main()
{
    if(!u_BlurEnabled)
    {
		vec3 color = texture(u_ScreenTexture, textureUV).rgb;		
		vec3 mapped = vec3(1.0) - exp(-color * u_Exposure);
		mapped = pow(mapped, vec3(1.0 / 2.2));
		gOut = vec4(mapped, 1.0);

		return;
    }

    vec3 sampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        sampleTex[i] = vec3(texture(u_ScreenTexture, textureUV.st + offsets[i]));
    }

    vec3 color = vec3(0.0);
    for(int i = 0; i < 9; i++)
    {
        color += sampleTex[i] * kernel[i];
    }
	
	vec3 mapped = vec3(1.0) - exp(-color * u_Exposure);
	mapped = pow(mapped, vec3(1.0 / 2.2));
    gOut = vec4(mapped, 1.0);
}