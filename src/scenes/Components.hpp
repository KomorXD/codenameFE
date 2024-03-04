#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>

class Texture;

struct TagComponent
{
	std::string Tag;

	TagComponent() = default;
	TagComponent(const TagComponent& other) = default;
};

struct TransformComponent
{
	glm::vec3 Position = glm::vec3(0.0f);
	glm::vec3 Rotation = glm::vec3(0.0f);
	glm::vec3 Scale	   = glm::vec3(1.0f);

	TransformComponent() = default;
	TransformComponent(const TransformComponent& other) = default;

	glm::mat4 ToMat4() const;
};

struct MaterialComponent
{
	glm::vec4 Color = glm::vec4(1.0f);
	float Shininess = 0.5f;
	std::shared_ptr<Texture> AlbedoTexture;

	MaterialComponent() = default;
	MaterialComponent(const MaterialComponent& other) = default;
};

struct MeshComponent
{
	enum class MeshType
	{
		PLANE,
		CUBE,
		LINE
	};

	MeshType Type;

	MeshComponent() = default;
	MeshComponent(const MeshComponent& other) = default;
};

struct DirectionalLightComponent
{
	glm::vec3 Direction = glm::vec3(-0.5f, -1.0f, -0.2f);
	glm::vec3 Color		= glm::vec3(1.0f);

	DirectionalLightComponent() = default;
	DirectionalLightComponent(const DirectionalLightComponent& other) = default;
};

struct PointLightComponent
{
	glm::vec3 Color = glm::vec3(1.0f);

	float LinearTerm	= 0.022f;
	float QuadraticTerm = 0.0019f;

	PointLightComponent() = default;
	PointLightComponent(const PointLightComponent& other) = default;
};

struct SpotLightComponent
{
	glm::vec3 Direction = glm::vec3(0.0f, -1.0f, 0.0f);
	glm::vec3 Color		= glm::vec3(1.0f);

	float Cutoff	  = 12.5f;
	float OuterCutoff = 17.5;

	float LinearTerm	= 0.022f;
	float QuadraticTerm = 0.0019f;

	SpotLightComponent() = default;
	SpotLightComponent(const SpotLightComponent& other) = default;
};