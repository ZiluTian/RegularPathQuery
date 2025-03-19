#include <iostream>

// Simple testing framework
#define ASSERT_EQ(actual, expected) \
    if ((actual) != (expected)) { \
        std::cerr << "Test failed: " << #actual << " != " << #expected << " (" << (actual) << " != " << (expected) << ")" << std::endl; \
        return false; \
    }

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        std::cerr << "Test failed: " << #condition << " is false" << std::endl; \
        return false; \
    }

#define ASSERT_FALSE(condition) \
    if ((condition)) { \
        std::cerr << "Test failed: " << #condition << " is true" << std::endl; \
        return false; \
    }


#define RUN_TEST(test) \
    if (test()) { \
        std::cout << "Test passed: " << #test << std::endl; \
    } else { \
        std::cerr << "Test failed: " << #test << std::endl; \
    }