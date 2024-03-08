#version 430 core

layout(location = 0)  in vec3 a_Pos;
layout(location = 1)  in vec3 a_Normal;
layout(location = 2)  in vec3 a_Tangent;
layout(location = 3)  in vec3 a_Bitangent;
layout(location = 4)  in vec2 a_TextureUV;
layout(location = 5)  in mat4 a_Transform;
layout(location = 9)  in vec4 a_Color;
layout(location = 10) in vec2 a_TilingFactor;
layout(location = 11) in vec2 a_TextureOffset;
layout(location = 12) in float a_TextureSlot;
layout(location = 13) in float a_NormalTextureSlot;

layout (std140, binding = 0) uniform Matrices
{
	mat4 u_Projection;
	mat4 u_View;
};

out VS_OUT
{
	vec3 worldPos;
	mat3 TBN;
	vec4 color;
	vec2 textureUV;
	flat float textureSlot;
	flat float normalTextureSlot;
} vs_out;

void main()
{
	vec3 T = normalize(vec3(a_Transform * vec4(a_Tangent,   0.0)));
	vec3 B = normalize(vec3(a_Transform * vec4(a_Bitangent, 0.0)));
	vec3 N = normalize(vec3(a_Transform * vec4(a_Normal,    0.0)));

	vs_out.worldPos			 = (a_Transform * vec4(a_Pos, 1.0)).xyz;
	vs_out.TBN				 = mat3(T, B, N);
	vs_out.color			 = a_Color;
	vs_out.textureUV		 = a_TextureUV * a_TilingFactor + a_TextureOffset;
	vs_out.textureSlot		 = a_TextureSlot;
	vs_out.normalTextureSlot = a_NormalTextureSlot;

	gl_Position = u_Projection * u_View * a_Transform * vec4(a_Pos, 1.0);
}