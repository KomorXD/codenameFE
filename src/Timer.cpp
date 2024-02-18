#include "Timer.hpp"
#include "Timer.hpp"
#include "Timer.hpp"
#include "Timer.hpp"
#include "Timer.hpp"
#include "Logger.hpp"

Timer::Timer(const std::string& taskName)
	: m_Name(taskName)
{
	LOG_INFO("Started {}.", m_Name);
	m_StartTS = std::chrono::steady_clock::now();
}

Timer::~Timer()
{
	float result = 0.0f;

	if (m_IsRunning)
	{
		result = std::chrono::duration<float>(std::chrono::steady_clock::now() - m_StartTS).count() * 1000.0f;
	}

	LOG_INFO("{} finished after {:.2f}ms.", m_Name, result + m_AccumulatedTime);
}

void Timer::Start()
{
	m_IsRunning = true;
	m_StartTS = std::chrono::steady_clock::now();
}

void Timer::Stop()
{
	m_AccumulatedTime += std::chrono::duration<float>(std::chrono::steady_clock::now() - m_StartTS).count() * 1000.0f;
	m_IsRunning = false;
}

void Timer::Restart()
{
	m_AccumulatedTime = 0.0f;
	Start();
}

float Timer::GetElapsedTime() const
{
	if (!m_IsRunning)
	{
		return m_AccumulatedTime;
	}

	return m_AccumulatedTime + std::chrono::duration<float>(std::chrono::steady_clock::now() - m_StartTS).count() * 1000.0f;
}
