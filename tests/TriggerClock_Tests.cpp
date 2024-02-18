#include <gtest/gtest.h>
#include <thread>

#include "TriggerClock.hpp"

using namespace std::chrono_literals;

TEST(TriggerClock, ActionFiring)
{
	TriggerClock::UpdateClocks();

	bool fired = false;
	TriggerClock clock([&fired]() { fired = true; });
	clock.SetInterval(300);
	clock.Start();

	std::this_thread::sleep_for(100ms);
	TriggerClock::UpdateClocks();

	ASSERT_FALSE(fired) << "Action fired too early.";

	std::this_thread::sleep_for(250ms);
	TriggerClock::UpdateClocks();

	ASSERT_TRUE(fired) << "Action did not fire in time.";
}

TEST(TriggerClock, ClockStopping)
{
	TriggerClock::UpdateClocks();

	bool fired = false;
	TriggerClock clock([&fired]() { fired = true; });
	clock.SetInterval(300);
	clock.Start();

	std::this_thread::sleep_for(100ms);
	TriggerClock::UpdateClocks();

	ASSERT_FALSE(fired) << "Action fired too early.";

	clock.Stop();
	std::this_thread::sleep_for(250ms);
	TriggerClock::UpdateClocks();
	
	ASSERT_FALSE(fired) << "Clock did not stop.";

	clock.Start();
	std::this_thread::sleep_for(250ms);
	TriggerClock::UpdateClocks();

	ASSERT_TRUE(fired) << "Action did not fire in time.";
}

TEST(TriggerClock, ClockRestarting)
{
	TriggerClock::UpdateClocks();

	bool fired = false;
	TriggerClock clock([&fired]() { fired = true; });
	clock.SetInterval(300);
	clock.Start();

	std::this_thread::sleep_for(200ms);
	TriggerClock::UpdateClocks();

	ASSERT_FALSE(fired) << "Action fired too early.";

	clock.Restart();
	std::this_thread::sleep_for(200ms);
	TriggerClock::UpdateClocks();

	ASSERT_FALSE(fired) << "Clock did not reset, hence the action triggered.";

	std::this_thread::sleep_for(200ms);
	TriggerClock::UpdateClocks();

	ASSERT_TRUE(fired) << "Action did not fire in time.";
}

TEST(TriggerClock, ClockCatchingUp)
{
	TriggerClock::UpdateClocks();

	int32_t val = 0;
	TriggerClock clock([&val]() { val++; });
	clock.SetInterval(200);
	clock.Start();

	std::this_thread::sleep_for(450ms);
	TriggerClock::UpdateClocks();
	ASSERT_EQ(val, 2) << "Clock did not catch up to 1 missed trigger.";

	val = 0;
	clock.Restart();
	std::this_thread::sleep_for(700ms);
	TriggerClock::UpdateClocks();
	ASSERT_EQ(val, 3) << "Clock did not catch up to 2 missed triggers.";
}