#include <gtest/gtest.h>

#include "RandomUtils.hpp"

TEST(StringUtils, StringReplace)
{
	std::string s = "AAA ${TOKEN} CCC";
	Replace(s, "${TOKEN}", "BBB");
	EXPECT_EQ(s, "AAA BBB CCC") << "AAA BBB CCC expected, got: " << s;

	s = "123";
	Replace(s, "${TOKEN2}", "1");
	EXPECT_EQ(s, "123") << "123 expected, but somehow changed to: " << s;

	s = "kappa ${TOKEN} deluxe ${TOKEN}";
	Replace(s, "${TOKEN}", "chungus");
	EXPECT_EQ(s, "kappa chungus deluxe ${TOKEN}") << "kappa chungus deluxe ${TOKEN} expected, got: " << s;

	s = "";
	Replace(s, "a", "b");
	EXPECT_EQ(s, "") << "Empty string replaced?";

	s = "";
	Replace(s, "", "b");
	EXPECT_EQ(s, "") << "Empty pattern recognized?";
}

TEST(StringUtils, StringReplaceAll)
{
	std::string s = "AAA ${TOKEN} CCC";
	ReplaceAll(s, "${TOKEN}", "BBB");
	EXPECT_EQ(s, "AAA BBB CCC") << "AAA BBB CCC expected, got: " << s;

	s = "123";
	ReplaceAll(s, "${TOKEN2}", "1");
	EXPECT_EQ(s, "123") << "123 expected, but somehow changed to: " << s;

	s = "kappa ${TOKEN} deluxe ${TOKEN}";
	ReplaceAll(s, "${TOKEN}", "chungus");
	EXPECT_EQ(s, "kappa chungus deluxe chungus") << "kappa chungus deluxe chungus expected, got: " << s;

	s = "";
	ReplaceAll (s, "a", "b");
	EXPECT_EQ(s, "") << "Empty string replaced?";

	s = "";
	ReplaceAll(s, "", "b");
	EXPECT_EQ(s, "") << "Empty pattern recognized?";
}

TEST(VectorUtils, MaxComponent)
{
	glm::vec2 v2(1.0f, 2.0f);
	EXPECT_EQ(MaxComponent(v2), 2.0f) << "expected 2.0, got: " << MaxComponent(v2);
	v2 = { -2.0f, 1.0f };
	EXPECT_EQ(MaxComponent(v2), 1.0f) << "expected 1.0, got: " << MaxComponent(v2);

	glm::vec3 v3(1.0f, 2.0f, 3.0f);
	EXPECT_EQ(MaxComponent(v3), 3.0f) << "expected 3.0, got: " << MaxComponent(v3);
	v3 = { -2.0f, 5.0f, 0.0f };
	EXPECT_EQ(MaxComponent(v3), 5.0f) << "expected 5.0, got: " << MaxComponent(v3);

	glm::vec4 v4(1.0f, 3.0f, -1.0f, -10.0f);
	EXPECT_EQ(MaxComponent(v4), 3.0f) << "expected 3.0, got: " << MaxComponent(v4);
	v4 = { -2.0f, 5.0f, 0.0f, -20.0f };
	EXPECT_EQ(MaxComponent(v4), 5.0f) << "expected 5.0, got: " << MaxComponent(v4);
}