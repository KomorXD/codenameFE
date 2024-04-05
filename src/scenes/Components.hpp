#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>

#include "../renderer/AssetManager.hpp"

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
	int32_t MaterialID = AssetManager::MATERIAL_DEFAULT;

	MaterialComponent() = default;
	MaterialComponent(const MaterialComponent& other) = default;
};

struct MeshComponent
{
	int32_t MeshID = AssetManager::MESH_CUBE;

	MeshComponent() = default;
	MeshComponent(const MeshComponent& other) = default;
};

struct DirectionalLightComponent
{
	glm::vec3 Color	= glm::vec3(1.0f);
	float Intensity = 1.0f;

	DirectionalLightComponent() = default;
	DirectionalLightComponent(const DirectionalLightComponent& other) = default;
};

struct PointLightComponent
{
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity	= 1.0f;

	float LinearTerm	= 0.022f;
	float QuadraticTerm = 0.0019f;

	PointLightComponent() = default;
	PointLightComponent(const PointLightComponent& other) = default;
};

struct SpotLightComponent
{
	glm::vec3 Color	= glm::vec3(1.0f);
	float Intensity = 1.0f;

	float Cutoff = 12.5f;
	float EdgeSmoothness = 0.0f;

	float LinearTerm	= 0.022f;
	float QuadraticTerm = 0.0019f;

	SpotLightComponent() = default;
	SpotLightComponent(const SpotLightComponent& other) = default;
};