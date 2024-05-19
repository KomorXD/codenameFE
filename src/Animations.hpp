#pragma once

#include <glm/glm.hpp>
#include <functional>

enum class AnimType
{
	Linear,
	EaseIn,
	EaseOut,
	EaseInOut
};

class Animations
{
public:
	static void DoFloat(float& value, float target, float duration, AnimType type);

	static void DoVec2(glm::vec2& value, const glm::vec2& target, float duration, AnimType type);
	static void DoVec3(glm::vec3& value, const glm::vec3& target, float duration, AnimType type);
	static void DoVec4(glm::vec4& value, const glm::vec4& target, float duration, AnimType type);

	static void DoMat3(glm::mat3& value, const glm::mat3& target, float duration, AnimType type);
	static void DoMat4(glm::mat4& value, const glm::mat4& target, float duration, AnimType type);

private:
	Animations() = delete;
	Animations(const Animations& other) = delete;
	Animations(Animations&& other) = delete;

	static void Update(float ts);

	static float Linear(float a, float b, float t);
	static float EaseIn(float a, float b, float t);
	static float EaseOut(float a, float b, float t);
	static float EaseInOut(float a, float b, float t);

	struct AnimItem
	{
		float* Value;
		float InitialValue{};
		float TargetValue{};

		std::function<float(float, float, float)> TransformFunc;

		float Duration = 1.0f;
		float Timestamp = 0.0f;
	};

	static std::vector<AnimItem> s_ActiveAnimations;

	friend class Application;
};