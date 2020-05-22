#ifndef _GTEST_TEST_ENV_H_
#define _GTEST_TEST_ENV_H_

#include <gtest/gtest.h>
/**
 * @file test_env.h
 * @synopsis 测试的全局事件
 * @author Carl, v12know@hotmail.com
 * @version 1.0.0
 * @date 2017-01-09
 */

namespace gtest {
    class TestEnv : public testing::Environment {
    public:
        TestEnv() {}

        virtual void SetUp() override;

        virtual void TearDown() override;

    private:
    };
} /* gtest */


#endif /* end of include guard: _GTEST_TEST_ENV_H_ */
