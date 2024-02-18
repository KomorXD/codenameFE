#pragma once

#include <chrono>

class Timer
{
private:
	std::chrono::steady_clock::time_point m_StartTS;
	std::string m_Name;

	float m_AccumulatedTime = 0.0f;
	bool m_IsRunning = true;

public:
	Timer(const std::string& taskName);
	~Timer();

	void Start();
	void Stop();
	void Restart();

	float GetElapsedTime() const;
};

#ifdef TARGET_WINDOWS
#define FUNC_NAME __func__
#else
#define FUNC_NAME __PRETTY_FUNCTION__
#endif

#ifndef CONF_PROD
#define SCOPE_PROFILE(name) Timer t##__LINE__(name)
#define FUNC_PROFILE() Timer t##FUNC_NAME(FUNC_NAME)
#else
#define SCOPE_PROFILE(name)
#define FUNC_PROFILE()
#endif