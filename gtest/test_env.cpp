#include "test_env.h"

#include "base/redis.h"

using namespace base::redis;

namespace gtest {
    void TestEnv::SetUp() {
        std::cout << "Foo FooEnvironment SetUp" << std::endl;
        RedisFactory::create_redis("localhost", 6379, 0, "");
    }

    void TestEnv::TearDown() {
        std::cout << "Foo FooEnvironment TearDown" << std::endl;
    }

} /* test */
