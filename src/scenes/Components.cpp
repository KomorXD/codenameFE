#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Components.hpp"
#include "../renderer/OpenGL.hpp"

glm::mat4 TransformComponent::ToMat4() const
{
	return glm::translate(glm::mat4(1.0f), Position)
		* glm::toMat4(glm::quat(Rotation))
		* glm::scale(glm::mat4(1.0f), Scale);
}
