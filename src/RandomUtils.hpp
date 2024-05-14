#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

std::vector<glm::vec4> FrustumCornersWorldSpace(const glm::mat4& projView);
bool TransformDecompose(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
std::optional<std::string> OpenFileDialog(const std::string& directory);
