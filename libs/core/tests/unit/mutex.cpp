//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

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
