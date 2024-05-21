#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

std::vector<glm::vec4> FrustumCornersWorldSpace(const glm::mat4& projView);
bool TransformDecompose(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
std::optional<std::string> OpenFileDialog(const std::string& directory);

void Replace(std::string& source, const std::string& pattern, const std::string& replacement);
void ReplaceAll(std::string& source, const std::string& pattern, const std::string& replacement);

float MaxComponent(const glm::vec2& vec);
float MaxComponent(const glm::vec3& vec);
float MaxComponent(const glm::vec4& vec);

float LightRadius(float constantTerm, float linearTerm, float quadraticTerm, float maxBrightness);
