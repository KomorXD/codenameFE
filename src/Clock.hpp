#pragma once

#include <chrono>

class Clock 
{
private:
	std::chrono::steady_clock::time_point m_StartTS;
	float m_AccumulatedTime = 0.0f;
	bool m_IsRunning = true;

public:
	Clock();

	void Start();
	void Stop();
	void Restart();

	uint32_t GetElapsedTime() const;
};
