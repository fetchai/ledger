//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#ifdef FETCH_DEBUG_MUTEX

using fetch::Mutex;
using fetch::RMutex;

TEST(DebugMutex, SimpleProblem)
{
  fetch::DeadlockHandler::ThrowOnDeadlock();
  {
    Mutex                  mutex;
    std::lock_guard<Mutex> guard1(mutex);
    EXPECT_THROW(std::lock_guard<Mutex> guard2(mutex), std::runtime_error);
  }

  {
    Mutex                  mutex1;
    Mutex                  mutex2;
    std::lock_guard<Mutex> guard1(mutex1);
    std::lock_guard<Mutex> guard2(mutex2);

    EXPECT_THROW(std::lock_guard<Mutex> guard3(mutex2), std::runtime_error);
  }
}

TEST(DebugMutex, MultiThreadDeadlock)
{
  fetch::DeadlockHandler::ThrowOnDeadlock();
  Mutex m[5];
  auto  f = [&m](int32_t n) {
    std::lock_guard<Mutex> guard1(m[n]);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (n != 0)
    {
      std::lock_guard<Mutex> guard2(m[n - 1]);
    }
  };

  std::vector<std::thread> threads;

  {
    std::lock_guard<Mutex> guard1(m[0]);
    threads.emplace_back(f, 1);
    threads.emplace_back(f, 2);
    threads.emplace_back(f, 3);
    threads.emplace_back(f, 4);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    EXPECT_THROW(std::lock_guard<Mutex> guard2(m[4]), std::runtime_error);
  }
  //  t0.join();
  threads[0].join();
  threads[1].join();
  threads[2].join();
  threads[3].join();
  threads.clear();
}

TEST(DebugMutex, DISABLED_MultiThreadDeadlock2)
{
  fetch::DeadlockHandler::AbortOnDeadlock();
  Mutex m[5];
  auto  f = [&m](int32_t n) {
    FETCH_LOCK(m[n]);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (n != 0)
    {
      FETCH_LOCK(m[n - 1]);
    }
  };

  std::vector<std::thread> threads;

  {
    FETCH_LOCK(m[0]);
    threads.emplace_back(f, 1);
    threads.emplace_back(f, 2);
    threads.emplace_back(f, 3);
    threads.emplace_back(f, 4);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    EXPECT_THROW(FETCH_LOCK(m[4]), std::runtime_error);
  }
  //  t0.join();
  threads[0].join();
  threads[1].join();
  threads[2].join();
  threads[3].join();
  threads.clear();
}

TEST(DebugMutex, CorrectRecursive)
{
  fetch::DeadlockHandler::ThrowOnDeadlock();
  RMutex m;

  EXPECT_TRUE(m.try_lock());
  EXPECT_TRUE(m.try_lock());
  EXPECT_TRUE(m.try_lock());
  EXPECT_TRUE(m.try_lock());
  m.unlock();
  m.unlock();
  m.unlock();
  m.unlock();

  std::string rv;

  auto f = [&m, &rv](char c) {
    m.lock();
    m.lock();
    m.lock();
    m.lock();

    rv += c;
    rv += c;

    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    m.unlock();
    m.unlock();

    rv += c;
    rv += c;

    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    m.unlock();
    m.unlock();
  };

  std::vector<std::thread> threads;
  threads.emplace_back(f, 'a');
  threads.emplace_back(f, 'b');

  threads[0].join();
  threads[1].join();

  ASSERT_TRUE(rv == "aaaabbbb" || rv == "bbbbaaaa");
}

TEST(DebugMutex, IncorrectRecursive)
{
  fetch::DeadlockHandler::ThrowOnDeadlock();
  RMutex m;

  std::vector<std::thread> threads;

  threads.emplace_back([&m] {
    FETCH_LOCK(m);
    FETCH_LOCK(m);
    std::this_thread::sleep_for(std::chrono::seconds(4));
  });

  threads.emplace_back([&m] {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    EXPECT_THROW(std::lock_guard<RMutex> failed_guard(m), std::runtime_error);
  });

  for (auto &t : threads)
  {
    t.join();
  }
}

#endif  // FETCH_DEBUG_MUTEX
