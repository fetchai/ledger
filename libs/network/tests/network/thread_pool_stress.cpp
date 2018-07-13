#include <cstdlib>
#include <iostream>
#include <memory>
#include"network/details/thread_pool.hpp"

using namespace fetch::network;

template< std::size_t N = 1>
void TestCase1() {
  std::cout << "TEST CASE 1. Threads: " << N << std::endl;
  std::cout << "Info: Testing thread manager starting, stopping and posting" << std::endl;

  {
      std::cout << "Info: Testing thread manager starting" << std::endl;
      auto tmanager = MakeThreadPool(N);
      tmanager -> Start();
  }

  {
      std::cout << "Info: Testing thread manager starting, stop, posting" << std::endl;
      auto tmanager = MakeThreadPool(N);
      tmanager -> Start();

      tmanager -> Post([tmanager]() { tmanager -> Stop(); });
      tmanager -> Stop();
  }

  {
      std::cout << "Info: Testing thread manager starting, post, activity, stop" << std::endl;
      auto tmanager = MakeThreadPool(N);
      tmanager -> Start();

      tmanager -> Post([]() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); });
      tmanager -> Post([]() { std::cout << "This thread prints stuff" << std::endl;; });
      tmanager -> Stop();
  }

  std::cout << "Success." << std::endl << std::endl;
}

template< std::size_t N = 1>
void TestCase3() {
  std::cout << "TEST CASE 3. Threads: " << N << std::endl;
  std::cout << "Info: Testing thread manager thread starvation/balancing" << std::endl;

  for (std::size_t index = 0; index < 10; ++index)
  {
    {
      auto tmanager = MakeThreadPool(N);
      tmanager -> Start();

      std::vector<int> ints{0,0,0,0};
      int testRunning = 0;

      for (std::size_t k = 0; k < ints.size(); ++k)
      {
        tmanager -> Post([&ints, &testRunning, k]() {

            while(testRunning < 1) { std::this_thread::sleep_for(std::chrono::milliseconds(10));}

            while(testRunning == 1)
            {
              ints[k]++;
            }

        });
      }

      testRunning = 1;

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      testRunning = 2;
      std::cout << "Stopping TM" << std::endl;
      tmanager -> Stop();
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

int main(int argc, char* argv[]) {

  TestCase1<1>();
  TestCase3<1>();

  TestCase1<10>();
  TestCase3<10>();

  std::cerr << "finished all tests" << std::endl;
  return 0;
}
