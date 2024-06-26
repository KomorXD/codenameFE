#version 430 core

layout(location = 0)  in vec3  a_Pos;
layout(location = 1)  in vec3  a_Normal;
layout(location = 2)  in vec3  a_Tangent;
layout(location = 3)  in vec3  a_Bitangent;
layout(location = 4)  in vec2  a_TextureUV;
layout(location = 5)  in mat4  a_Transform;
layout(location = 9)  in float a_MaterialSlot;
layout(location = 10) in float a_EntityID;

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

out VS_OUT
{
	vec3 worldPos;
	vec3 viewSpacePos;
	vec3 eyePos;
	vec3 normal;
	mat3 TBN;
	vec3 tangentWorldPos;
	vec3 tangentViewPos;
	vec2 textureUV;
	flat float materialSlot;
	flat float entityID;
} vs_out;

void main()
{
	vec3 T = normalize(mat3(a_Transform) * a_Tangent);
	vec3 B = normalize(mat3(a_Transform) * a_Bitangent);
	vec3 N = normalize(mat3(a_Transform) * a_Normal);
	mat3 TBN = transpose(mat3(T, B, N));

	vs_out.worldPos		   = (a_Transform * vec4(a_Pos, 1.0)).xyz;
	vs_out.viewSpacePos	   = (u_Camera.view * vec4(vs_out.worldPos, 1.0)).xyz;
	vs_out.eyePos		   = u_Camera.position.xyz;
	vs_out.normal		   = a_Normal;
	vs_out.TBN			   = TBN;
	vs_out.tangentWorldPos = TBN * vs_out.worldPos;
	vs_out.tangentViewPos  = TBN * u_Camera.position.xyz;
	vs_out.textureUV	   = a_TextureUV;
	vs_out.materialSlot	   = a_MaterialSlot;
	vs_out.entityID		   = a_EntityID;

	gl_Position = u_Camera.projection * u_Camera.view * a_Transform * vec4(a_Pos, 1.0);
}