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
#include <functional>
#include <thread>
#include <vector>

using Mutex  = fetch::SimpleDebugMutex;
using RMutex = fetch::RecursiveDebugMutex;

using namespace std::chrono_literals;

TEST(DebugMutex, SimpleProblem)
{
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

  auto runnable = [&hungry_philosophers](auto left_fork, auto right_fork) {
    FETCH_LOCK(left_fork.get());  // everybody, get and lock your forks
    std::this_thread::sleep_for(100ms);
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
  {
    fetch::DeadlockHandler::ThrowOnDeadlock();
    fetch::RecursiveLockAttempt::SetTimeoutMs(100);
    RMutex m;

    std::vector<std::thread> threads;

    threads.emplace_back([&m] {
      FETCH_LOCK(m);
      FETCH_LOCK(m);
      std::this_thread::sleep_for(300ms);
    });

    threads.emplace_back([&m] {
      std::this_thread::sleep_for(200ms);
      EXPECT_THROW(std::lock_guard<RMutex> failed_guard(m), std::runtime_error);
    });

    for (auto &t : threads)
    {
      t.join();
    }
  }

  {
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

  {
    fetch::DeadlockHandler::ThrowOnDeadlock();
    fetch::RecursiveLockAttempt::SetTimeoutMs(300);
    RMutex m;

    std::vector<std::thread> threads;

    std::atomic<std::size_t> visited{0};

    threads.emplace_back([&m] {
      FETCH_LOCK(m);
      std::this_thread::sleep_for(200ms);
    });

    threads.emplace_back([&m, &visited] {
      std::this_thread::sleep_for(100ms);
      FETCH_LOCK(m);
      ++visited;
    });

    for (auto &t : threads)
    {
      t.join();
    }

    EXPECT_EQ(visited, 1);
  }
}
