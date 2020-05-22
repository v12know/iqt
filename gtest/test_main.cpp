#include <gtest/gtest.h>
//#include "gmock/gmock.h"
#include <ostream>
#include "base/util.hpp"

#include "test_env.h"


int Foo(int a, int b)
{
    if (a == 0 || b == 0)
    {
        throw "don't do that";
    }
    int c = a % b;
    if (c == 0)
        return b;
    return Foo(b, c);
}

TEST(FooTest, HandleNoneZeroInput)
{
    EXPECT_EQ(2, Foo(4, 10));
    EXPECT_EQ(6, Foo(30, 18));
	//EXPECT_EQ(5, Foo(30, 18));
    std::cout << base::to_string(3.1415926222332234432324) << std::endl;
}
using namespace std;
int main(int argc, char* argv[])
{
/*	std::string confPath = "./config/single_grid_m_dev.xml";
	if (argc >= 2) {
		confPath = argv[1];
	}
	cerr << "confPath=" << confPath << endl;*/
	testing::AddGlobalTestEnvironment(new gtest::TestEnv());
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
