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

#include "network/management/network_manager.hpp"
#include <cstdlib>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>

using namespace fetch::network;

template <std::size_t N = 1>
void TestCase1()
{
  std::cout << "TEST CASE 1. Threads: " << N << std::endl;
  std::cout << "Info: Testing thread manager starting, stopping and posting" << std::endl;

  {
    NetworkManager tmanager{"NetMgr", N};
    tmanager.Start();
  }

  {
    NetworkManager tmanager{"NetMgr", N};
    tmanager.Start();

    // Don't post a stop to the original tmanager into itself or it will break
    NetworkManager tmanagerCopy = tmanager;

    tmanager.Post([&tmanagerCopy]() { tmanagerCopy.Stop(); });
    tmanager.Stop();
  }

  {
    NetworkManager tmanager{"NetMgr", N};
    tmanager.Start();

    tmanager.Post([]() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); });
    tmanager.Post([]() {
      std::cout << "This thread prints stuff" << std::endl;
      ;
    });
    tmanager.Stop();
  }

  SUCCEED() << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase3()
{
  std::cout << "TEST CASE 3. Threads: " << N << std::endl;
  std::cout << "Info: Testing thread manager thread starvation/balancing" << std::endl;

  for (std::size_t index = 0; index < 10; ++index)
  {
    {
      NetworkManager tmanager{"NetMgr", N};
      tmanager.Start();

      std::vector<int> ints{0, 0, 0, 0};
      int              testRunning = 0;

      for (std::size_t k = 0; k < ints.size(); ++k)
      {
        tmanager.Post([&ints, &testRunning, k]() {
          // while(testRunning < 1) {} // this never stops for some reason
          while (testRunning < 1)
          {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
          }

          while (testRunning == 1)
          {
            ints[k]++;
          }
        });
      }

      testRunning = 1;

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      testRunning = 2;
      std::cout << "Stopping TM" << std::endl;
      tmanager.Stop();
      std::cout << "Stopped TM" << std::endl;

      std::cout << "Thread workload: ";

      for (auto &i : ints)
      {
        std::cout << i << " ";
      }
      std::cout << std::endl;
    }
  }
  SUCCEED() << "Success." << std::endl;
}

template <std::size_t N = 1>
void TestCase4()
{
  std::cout << "TEST CASE 4. Threads: " << N << std::endl;
  std::cout << "Info: Stopping thread manager through its own post mechanism" << std::endl;
  for (std::size_t i = 0; i < 1000; ++i)
  {
    NetworkManager tmanager{"NetMgr", N};
    tmanager.Start();
    tmanager.Post([&tmanager]() { tmanager.Stop(); });
  }
  SUCCEED() << "Success." << std::endl;
}

TEST(thread_manager_stress_test, basic_test)
{

  TestCase1<1>();
  TestCase3<1>();
  // TestCase4<1>(); // fails

  TestCase1<10>();
  TestCase3<10>();
  // TestCase4<10>();
}
