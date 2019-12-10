#include "core/mutex.hpp"

#include "gmock/gmock.h"

#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

TEST(DebugMutex, SimpleProblem)
{

  {
    fetch::DebugMutex                  mutex{__FILE__, __LINE__};
    std::lock_guard<fetch::DebugMutex> guard1(mutex);
    EXPECT_THROW(std::lock_guard<fetch::DebugMutex> guard2(mutex), std::runtime_error);
  }

  {
    fetch::DebugMutex                  mutex1{__FILE__, __LINE__};
    fetch::DebugMutex                  mutex2{__FILE__, __LINE__};
    std::lock_guard<fetch::DebugMutex> guard1(mutex1);
    std::lock_guard<fetch::DebugMutex> guard2(mutex2);

    EXPECT_THROW(std::lock_guard<fetch::DebugMutex> guard3(mutex2), std::runtime_error);
  }
}