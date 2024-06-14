#version 430 core

out vec4 fragColor;

in vec2 textureUV;

const float PI = 3.14159265359;

float radicalInverseVdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley(uint i, uint N)
{
	return vec2(float(i) / float(N), radicalInverseVdC(i));
}

vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness * roughness;
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

float geoSchlickGGX(float NV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom = NV;
    float denom = NV * (1.0 - k) + k;

    return nom / denom;
}

float geoSmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NV = max(dot(N, V), 0.0);
    float NL = max(dot(N, L), 0.0);
    float ggx2 = geoSchlickGGX(NV, roughness);
    float ggx1 = geoSchlickGGX(NL, roughness);

    return ggx1 * ggx2;
}

vec2 integrateBRDF(float NV, float roughness)
{
    vec3 V;
    V.x = sqrt(1.0 - NV * NV);
    V.y = 0.0;
    V.z = NV;

    float A = 0.0;
    float B = 0.0; 

    vec3 N = vec3(0.0, 0.0, 1.0);
    
    const uint SAMPLE_COUNT = 1024u;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H = importanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NL = max(L.z, 0.0);
        float NH = max(H.z, 0.0);
        float VH = max(dot(V, H), 0.0);

        if(NL > 0.0)
        {
            float G = geoSmith(N, V, L, roughness);
            float G_Vis = (G * VH) / (NH * NV);
            float Fc = pow(1.0 - VH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);

    return vec2(A, B);
}

void main()
{
	fragColor = vec4(integrateBRDF(textureUV.x, textureUV.y), 0.0, 1.0);
}