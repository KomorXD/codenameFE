#include "Animations.hpp"

#include <glm/gtc/constants.hpp>

std::vector<Animations::AnimItem> Animations::s_ActiveAnimations{};

void Animations::DoFloat(float& value, float target, float duration, AnimType type)
{
	std::function<float(float, float, float)> f{};
	switch (type)
	{
	case AnimType::Linear:
		f = [](float a, float b, float c)->float { return Linear(a, b, c); };
		break;
	case AnimType::EaseIn:
		f = [](float a, float b, float c)->float { return EaseIn(a, b, c); };
		break;
	case AnimType::EaseOut:
		f = [](float a, float b, float c)->float { return EaseOut(a, b, c); };
		break;
	case AnimType::EaseInOut:
		f = [](float a, float b, float c)->float { return EaseInOut(a, b, c); };
		break;
	default:
		assert(false && "Unsupported AnimType provided.");
		break;
	}

	AnimItem item{};
	item.Value = &value;
	item.InitialValue = value;
	item.TargetValue = target;
	item.TransformFunc = f;
	item.Duration = duration;

	std::erase_if(s_ActiveAnimations, [&](const AnimItem& item) { return item.Value == &value; });
	s_ActiveAnimations.push_back(item);
}

void Animations::DoVec2(glm::vec2& value, const glm::vec2& target, float duration, AnimType type)
{
	DoFloat(value.x, target.x, duration, type);
	DoFloat(value.y, target.y, duration, type);
}

void Animations::DoVec3(glm::vec3& value, const glm::vec3& target, float duration, AnimType type)
{
	DoFloat(value.x, target.x, duration, type);
	DoFloat(value.y, target.y, duration, type);
	DoFloat(value.z, target.z, duration, type);
}

void Animations::DoVec4(glm::vec4& value, const glm::vec4& target, float duration, AnimType type)
{
	DoFloat(value.x, target.x, duration, type);
	DoFloat(value.y, target.y, duration, type);
	DoFloat(value.z, target.z, duration, type);
	DoFloat(value.w, target.w, duration, type);
}

void Animations::DoMat3(glm::mat3& value, const glm::mat3& target, float duration, AnimType type)
{
	DoVec3(value[0], target[0], duration, type);
	DoVec3(value[1], target[1], duration, type);
	DoVec3(value[2], target[2], duration, type);
}

void Animations::DoMat4(glm::mat4& value, const glm::mat4& target, float duration, AnimType type)
{
	DoVec4(value[0], target[0], duration, type);
	DoVec4(value[1], target[1], duration, type);
	DoVec4(value[2], target[2], duration, type);
	DoVec4(value[3], target[3], duration, type);
}

void Animations::Update(float ts)
{
	for (auto it = s_ActiveAnimations.begin(); it != s_ActiveAnimations.end();)
	{
		if (it->Timestamp >= it->Duration)
		{
			*it->Value = it->TargetValue;
			it = s_ActiveAnimations.erase(it);
			continue;
		}

		it->Timestamp += ts;
		*it->Value = it->TransformFunc(it->InitialValue, it->TargetValue, it->Timestamp / it->Duration);
		++it;
	}
}

float Animations::Linear(float a, float b, float t)
{
	return a + t * (b - a);
}

float Animations::EaseIn(float a, float b, float t)
{
	float base = 1.0 - glm::cos((glm::pi<float>() * t) / 2.0f);
	return a + base * (b - a);
}

float Animations::EaseOut(float a, float b, float t)
{
	float base = glm::sin((glm::pi<float>() * t) / 2.0f);
	return a + base * (b - a);
}

float Animations::EaseInOut(float a, float b, float t)
{
	float base = -(glm::cos(glm::pi<float>() * t) - 1.0f) / 2.0f;
	return a + base * (b - a);
}
