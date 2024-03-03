#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

class Texture;

struct Transform
{
	glm::vec3 Position = glm::vec3(0.0f);
	glm::vec3 Rotation = glm::vec3(0.0f);
	glm::vec3 Scale	   = glm::vec3(1.0f);

	glm::mat4 ToMat4() const;
};

struct Material
{
	glm::vec4 Color = glm::vec4(1.0f);
	float Shininess = 0.5f;

	std::shared_ptr<Texture> AlbedoTexture;
};

struct DirectionalLight
{
	glm::vec3 Direction = glm::vec3(-0.5f, -1.0f, -0.2f);
	glm::vec3 Color		= glm::vec3(1.0f);
};

struct PointLight
{
	glm::vec3 Color = glm::vec3(1.0f);

	float LinearTerm	= 0.022f;
	float QuadraticTerm = 0.0019f;
};

struct SpotLight
{
	glm::vec3 Direction = glm::vec3(0.0f, -1.0f, 0.0f);
	glm::vec3 Color		= glm::vec3(1.0f);

	float Cutoff	  = 12.5f;
	float OuterCutoff = 17.5;

	float LinearTerm	= 0.022f;
	float QuadraticTerm = 0.0019f;
};