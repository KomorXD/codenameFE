#include <gtest/gtest.h>

#include "Logger.hpp"
#include "TriggerClock.hpp"

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);

	Logger::Init();
	TriggerClock::UpdateClocks();

	return RUN_ALL_TESTS();
}