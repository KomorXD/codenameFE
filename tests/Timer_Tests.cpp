#include <gtest/gtest.h>

#include "Logger.hpp"
#include "Timer.hpp"

using namespace std::chrono_literals;

#define MAX_ERROR_IN_MS 15.0f

TEST(Timer, PropelyMeasuresTime)
{
	Timer t("0.5 seconds test");
	std::this_thread::sleep_for(500ms);

	ASSERT_NEAR(t.GetElapsedTime(), 500.0f, MAX_ERROR_IN_MS) << "Time measure error.";
}

TEST(Timer, ProperlyStops)
{
	Timer t("Stop test");
	std::this_thread::sleep_for(100ms);
	t.Stop();
	std::this_thread::sleep_for(100ms);

	ASSERT_NEAR(t.GetElapsedTime(), 100.0f, MAX_ERROR_IN_MS) << "Stop did not stop properly.";
}

TEST(Timer, StoppingAndResuming)
{
	Timer t("Stop and resume test");
	std::this_thread::sleep_for(200ms);
	t.Stop();
	std::this_thread::sleep_for(200ms);

	EXPECT_NEAR(t.GetElapsedTime(), 200.0f, MAX_ERROR_IN_MS) << "Timer expected to stop, which it did not.";

	t.Start();
	std::this_thread::sleep_for(200ms);

	ASSERT_NEAR(t.GetElapsedTime(), 400.0f, MAX_ERROR_IN_MS) << "Timer expected to run ~400ms, which it did not.";
}

TEST(Timer, Restarting)
{
	Timer t("Restarting test");
	std::this_thread::sleep_for(300ms);
	
	EXPECT_NEAR(t.GetElapsedTime(), 300.0f, MAX_ERROR_IN_MS) << "Timer did not measure time properly.";

	t.Restart();
	std::this_thread::sleep_for(200ms);

	ASSERT_NEAR(t.GetElapsedTime(), 200.0f, MAX_ERROR_IN_MS) << "Timer expected to restart and run for ~200ms, which it did not.";
}