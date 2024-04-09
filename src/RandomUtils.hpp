#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <string>

bool TransformDecompose(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
std::optional<std::string> OpenFileDialog(const std::string& directory);
