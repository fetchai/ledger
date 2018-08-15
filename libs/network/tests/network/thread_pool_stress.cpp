//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include "network/details/thread_pool.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>

using namespace fetch::network;

template <std::size_t N = 1>
void TestCase1()
{
  std::cout << "TEST CASE 1. Threads: " << N << std::endl;
  std::cout << "Info: Testing thread manager starting, stopping and posting" << std::endl;

  {
    std::cout << "Info: Testing thread manager starting" << std::endl;
    auto tmanager = MakeThreadPool(N);
    tmanager->Start();
  }

  {
    std::cout << "Info: Testing thread manager starting, stop, posting" << std::endl;
    auto tmanager = MakeThreadPool(N);
    tmanager->Start();

    tmanager->Post([tmanager]() { tmanager->Stop(); });
    tmanager->Stop();
  }

  {
    std::cout << "Info: Testing thread manager starting, post, activity, stop" << std::endl;
    auto tmanager = MakeThreadPool(N);
    tmanager->Start();

    tmanager->Post([]() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); });
    tmanager->Post([]() {
      std::cout << "This thread prints stuff" << std::endl;
      ;
    });
    tmanager->Stop();
  }

  std::cout << "Success." << std::endl << std::endl;
}

template <std::size_t N = 1>
void TestCase1a()
{
  std::cout << "TEST CASE 1a. Threads: " << N << std::endl;
  std::cout << "Info: Testing thread manager starting, stopping and posting" << std::endl;

  {
    std::cout << "Info: Testing thread manager starting" << std::endl;
    auto tmanager = MakeThreadPool(N);
    tmanager->Start();
  }
  std::cout << "Success." << std::endl << std::endl;
}

template <std::size_t N = 1>
void TestCase1b()
{
  std::cout << "TEST CASE 1b. Threads: " << N << std::endl;
  std::cout << "Info: Testing thread manager starting, stopping and posting" << std::endl;

  {
    std::cout << "Info: Testing thread manager starting, stop, posting" << std::endl;
    auto tmanager = MakeThreadPool(N);
    tmanager->Start();

    tmanager->Post([tmanager]() { tmanager->Stop(); });
    tmanager->Stop();
  }

  std::cout << "Success." << std::endl << std::endl;
}

template <std::size_t N = 1>
void TestCase1c()
{
  std::cout << "TEST CASE 1c. Threads: " << N << std::endl;
  std::cout << "Info: Testing thread manager starting, stopping and posting" << std::endl;

  {
    std::cout << "Info: Testing thread manager starting, post, activity, stop" << std::endl;
    auto tmanager = MakeThreadPool(N);
    tmanager->Start();

    tmanager->Post([]() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); });
    tmanager->Post([]() {
      std::cout << "This thread prints stuff" << std::endl;
      ;
    });
    tmanager->Stop();
  }

  std::cout << "Success." << std::endl << std::endl;
}

template <std::size_t N = 1>
void TestCase3()
{
  std::cout << "TEST CASE 3. Threads: " << N << std::endl;
  std::cout << "Info: Testing thread manager thread starvation/balancing" << std::endl;

  for (std::size_t index = 0; index < 10; ++index)
  {
    {
      auto tmanager = MakeThreadPool(N);
      tmanager->Start();

      std::vector<int> ints{0, 0, 0, 0};
      int              testRunning = 0;

      for (std::size_t k = 0; k < ints.size(); ++k)
      {
        tmanager->Post([&ints, &testRunning, k]() {
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
      tmanager->Stop();
      std::cout << "Stopped TM" << std::endl;

      std::cout << "Thread workload: ";

      for (auto &i : ints)
      {
        std::cout << i << " ";
      }
      std::cout << std::endl;
    }
  }
  std::cout << "Success." << std::endl << std::endl;
}

int main(int argc, char *argv[])
{

  TestCase1<1>();
  TestCase3<1>();

  TestCase1a<10>();
  TestCase1b<10>();
  TestCase1c<10>();
  TestCase1<10>();
  TestCase3<10>();

  std::cerr << "finished all tests" << std::endl;
  return 0;
}
