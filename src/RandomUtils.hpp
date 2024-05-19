#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

std::vector<glm::vec4> FrustumCornersWorldSpace(const glm::mat4& projView);
bool TransformDecompose(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
std::optional<std::string> OpenFileDialog(const std::string& directory);
float MaxComponent(const glm::vec2& vec);
float MaxComponent(const glm::vec3& vec);
float MaxComponent(const glm::vec4& vec);
