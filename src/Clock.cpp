#include "Clock.hpp"

Clock::Clock()
{
	m_StartTS = std::chrono::steady_clock::now();
}

void Clock::Start()
{
	m_IsRunning = true;
	m_StartTS = std::chrono::steady_clock::now();
}

void Clock::Stop()
{
	m_AccumulatedTime += std::chrono::duration<float>(std::chrono::steady_clock::now() - m_StartTS).count() * 1000.0f;
	m_IsRunning = false;
}

void Clock::Restart()
{
	m_AccumulatedTime = 0.0f;
	Start();
}

uint32_t Clock::GetElapsedTime() const
{
	if (!m_IsRunning)
	{
		return static_cast<uint32_t>(m_AccumulatedTime);
	}

	return static_cast<uint32_t>(m_AccumulatedTime + std::chrono::duration<float>(std::chrono::steady_clock::now() - m_StartTS).count() * 1000.0f);
}
