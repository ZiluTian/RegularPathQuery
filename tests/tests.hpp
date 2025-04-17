#include <iostream>
#include <chrono>
#include "rpqdb/Profiler.hpp"

using namespace std::chrono;
using namespace rpqdb;

class QueryGraphClass {
    public:
    string query;
    string graph_name_prefix;

    public:
    // Constructor
    QueryGraphClass(const std::string& q, const std::string& prefix)
        : query(q), graph_name_prefix(prefix) {
        std::cout << "QueryGraphClass constructed.\n";
    }

    // Destructor
    ~QueryGraphClass() {
        std::cout << "QueryGraphClass destroyed.\n";
    }

    virtual void run(int size, const string& profile_name = "profile.dat") {}
};

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

template<typename Func, typename... Args>
long long benchmark(Func func, Args&&... args) {
    auto start = high_resolution_clock::now();
    func(std::forward<Args>(args)...);  // Execute the function
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}