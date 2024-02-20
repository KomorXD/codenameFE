#pragma once

#include <glm/glm.hpp>

struct DirectionalLight
{
	glm::vec3 Direction = { 0.0f, -1.0f, 0.0f };
	glm::vec3 Color		= glm::vec3(1.0f);
};

struct PointLight
{
	glm::vec3 Position  = glm::vec3(0.0f);
	glm::vec3 Color	    = glm::vec3(1.0f);
	float LinearTerm    = 0.09f;
	float QuadraticTerm = 0.032f;
};