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
#include "meta/value_util.hpp"
#include "moment/clocks.hpp"

#include "gmock/gmock.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <thread>
#include <vector>

using Mutex  = fetch::SimpleDebugMutex;
using RMutex = fetch::RecursiveDebugMutex;

using namespace std::chrono_literals;

namespace {

auto Clock()
{
  static auto ret_val = fetch::moment::CreateAdjustableClock("core:RecursiveLockAttempt");
  return ret_val;
}

}  // namespace

TEST(DebugMutex, SimpleProblem)
{
  /* Seven philosophers are seated behind a round table and are trying to eat spaghetti.
   * They only have seven forks interspersed between them.
   * Each one first takes a fork on his left and then tries to acquire one on his right.
   */
  fetch::DeadlockHandler::ThrowOnDeadlock();
  {
    Mutex                  mutex;
    std::lock_guard<Mutex> guard1(mutex);
    EXPECT_THROW(std::lock_guard<Mutex> guard2(mutex), std::runtime_error);
  }

  static constexpr std::size_t TABLE_SIZE = 7;

  std::vector<Mutex>       forks(TABLE_SIZE);
  std::vector<std::thread> dining_philosophers;
  std::atomic<std::size_t> hungry_philosophers{0};
  std::atomic<std::size_t> left_forks_wielded{0};

  auto runnable = [&hungry_philosophers, &left_forks_wielded](auto left_fork, auto right_fork) {
    FETCH_LOCK(left_fork.get());  // everybody, get and lock your forks
    ++left_forks_wielded;
    while (left_forks_wielded < TABLE_SIZE)
    {
      std::this_thread::sleep_for(1ms);
    }
    try
    {
      FETCH_LOCK(right_fork.get());
    }
    catch (...)
    {
      ++hungry_philosophers;
    }
  };

  for (std::size_t fork = 0; fork < TABLE_SIZE; ++fork)
  {
    Mutex &left_fork  = forks[fork];
    Mutex &right_fork = forks[(fork + 1) % TABLE_SIZE];
    dining_philosophers.emplace_back(runnable, std::ref(left_fork), std::ref(right_fork));
  }

  for (auto &philosopher : dining_philosophers)
  {
    philosopher.join();
  }

  ASSERT_EQ(hungry_philosophers, 1);
}

TEST(DebugMutex, DISABLED_MultiThreadDeadlock2)
{
  // Basically the same as in the test above but aborts rather than throw.
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

template <class... Bools>
inline void WaitFor(std::atomic<Bools> const &... until)
{
  while (!fetch::value_util::Accumulate(std::logical_or<void>{}, false, until...))
  {
    std::this_thread::sleep_for(1ms);
  }
  std::this_thread::sleep_for(1ms);
}

using Bool = std::atomic<bool>;

TEST(DebugMutex, CorrectRecursive)
{
  Clock()->AddOffset(0s);
  {
    // Two threads modify a single string synchronised through a recursive mutex.
    fetch::DeadlockHandler::ThrowOnDeadlock();
    fetch::RecursiveLockAttempt::SetTimeoutMs(5'000);
    RMutex m;

    std::atomic<std::size_t> level{0};
    for (std::size_t i{}; i < 4; ++i)
    {
      if (m.try_lock())
      {
        ++level;
      }
    }
    while (level-- > 0)
    {
      m.unlock();
    }

    std::string rv;

    auto f = [&m, &rv](char c) {
      std::this_thread::sleep_for(1ms);
      FETCH_LOCK(m);
      std::this_thread::sleep_for(1ms);
      FETCH_LOCK(m);
      {
        std::this_thread::sleep_for(1ms);
        FETCH_LOCK(m);
        std::this_thread::sleep_for(1ms);
        FETCH_LOCK(m);

        rv += c;
        rv += c;
      }
      rv += c;
      rv += c;
    };

    std::vector<std::thread> threads;
    threads.emplace_back(f, 'a');
    threads.emplace_back(f, 'b');

    for (auto &t : threads)
    {
      t.join();
    }

    ASSERT_TRUE(rv == "aaaabbbb" || rv == "bbbbaaaa");
  }
  {
    // A thread acquires a recursive mutex and holds it for a long time ...
    // luckily not long enough for the dispatcher to assume a deadlock.
    fetch::DeadlockHandler::ThrowOnDeadlock();
    fetch::RecursiveLockAttempt::SetTimeoutMs(200'000);
    RMutex m;

    std::vector<std::thread> threads;

    std::atomic<std::size_t> visited{0};
    Bool                     wake_first{false}, wake_second{false};

    threads.emplace_back([&m, &wake_first] {
      FETCH_LOCK(m);
      WaitFor(wake_first);
    });

    threads.emplace_back([&m, &visited, &wake_second] {
      WaitFor(wake_second);
      FETCH_LOCK(m);
      ++visited;
    });

    Clock()->AddOffset(100s);
    wake_second = true;
    Clock()->AddOffset(50s);
    wake_first = true;

    for (auto &t : threads)
    {
      t.join();
    }

    EXPECT_EQ(visited, 1);
  }
}

TEST(DebugMutex, IncorrectRecursive)
{
  Clock()->AddOffset(0s);
  {
    // A thread acquires a recursive mutex and holds it for way too long.
    // Some time later, another thread tries to acquire the same mutex.
    // The dispatcher notices the first thread has been holding it for way too long.
    fetch::DeadlockHandler::ThrowOnDeadlock();
    fetch::RecursiveLockAttempt::SetTimeoutMs(100'000);
    RMutex m;
    Bool   wake_first{false}, wake_second{false}, output_channel{false};

    std::vector<std::thread> threads;

    threads.emplace_back([&m, &wake_first, &output_channel] {
      FETCH_LOCK(m);
      FETCH_LOCK(m);
      output_channel = true;
      WaitFor(wake_first);
    });

    threads.emplace_back([&m, &wake_second] {
      WaitFor(wake_second);
      std::this_thread::sleep_for(10ms);
      EXPECT_THROW(std::lock_guard<RMutex> failed_guard(m), std::runtime_error);
    });

    WaitFor(output_channel);
    Clock()->AddOffset(200s);
    wake_second = true;
    Clock()->AddOffset(100s);

    threads[1].join();
    wake_first = true;
    threads[0].join();
  }

  {
    // A thread acquires a recursive mutex and holds it for way too long.
    // Another thread tries to acquire the same mutex and is blocked until mutex release.
    // Some time later, the dispatcher notices the second thread has been waiting for the mutex
    // for way too long.
    // Since RecursiveLockAttemtp uses std::recursive_timed_mutex::try_lock_for() it is unlikely
    // AdjustableClock can be properly used here.
    fetch::DeadlockHandler::ThrowOnDeadlock();
    fetch::RecursiveLockAttempt::SetTimeoutMs(200);
    RMutex m;

    std::vector<std::thread> threads;

    threads.emplace_back([&m] {
      FETCH_LOCK(m);
      std::this_thread::sleep_for(400ms);
    });

    threads.emplace_back([&m] {
      std::this_thread::sleep_for(100ms);
      EXPECT_THROW(std::lock_guard<RMutex> failed_guard(m), std::runtime_error);
    });

    for (auto &t : threads)
    {
      t.join();
    }
  }
}
