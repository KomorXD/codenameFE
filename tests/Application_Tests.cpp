#include <gtest/gtest.h>
#include <signal.h>

#include "Application.hpp"

TEST(Application, WindowCreated)
{
	volatile Application app;

	ASSERT_NE(app.Instance()->Window(), nullptr) << "Window was not created as it should.";
}

TEST(Application, MultipleAppGuard)
{
	volatile Application app1;
	
	ASSERT_DEATH({ volatile Application app2; }, "Only one instance of Application allowed!");
}

TEST(Application, SequentialAppCreation)
{
	{
		volatile Application app1;
	}

	ASSERT_NO_THROW(volatile Application app2) << "Application should not throw if the first instance is already dead.";
}