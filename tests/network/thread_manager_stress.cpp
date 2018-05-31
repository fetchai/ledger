#include <cstdlib>
#include <iostream>
#include <memory>
#include"network/thread_manager.hpp"

using namespace fetch::network;

template< std::size_t N = 1>
void TestCase1() {
  std::cout << "TEST CASE 1. Threads: " << N << std::endl;
  std::cout << "Info: Testing thread manager starting, stopping and posting" << std::endl;

  {
      ThreadManager tmanager(N);
      tmanager.Start();
  }

  {
      ThreadManager tmanager(N);
      tmanager.Start();

      tmanager.Post([&tmanager]() { tmanager.Stop(); });
      tmanager.Stop();
  }

  {
      ThreadManager tmanager(N);
      tmanager.Start();

      tmanager.Post([]() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); });
      tmanager.Post([]() { std::cout << "This thread prints stuff" << std::endl;; });
      tmanager.Stop();
  }

  std::cout << "Success." << std::endl << std::endl;
}

template< std::size_t N = 1>
void TestCase2() {
  std::cout << "TEST CASE 2. Threads: " << N << std::endl;
  std::cout << "Info: Testing thread manager operation when it is being moved" << std::endl;

  {
    ThreadManager tmanager(N);
    tmanager.Start();

    std::atomic<int>counter{0};

    tmanager.Post([&counter]() {
      for (std::size_t i = 0; i < 5; ++i)
      {
      counter++;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    });

    while(true)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if(counter == 5)
      {
        break;
      }

      std::cout << "Waiting for counter, test 2.0 -" << counter << std::endl;
    }

    tmanager.Stop();
  }

  {
    std::shared_ptr<ThreadManager> shared = std::make_shared<ThreadManager>(N);
    shared->Start();

    std::atomic<int>counter{0};

    shared->Post([&counter]() {
      for (std::size_t i = 0; i < 5; ++i)
      {
      counter++;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    });

    ThreadManager tm(std::move(*shared));
    shared.reset();

    while(true)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if(counter == 5)
      {
        break;
      }

      std::cout << "Waiting for counter, test 2.1 -" << counter << std::endl;
    }
  }

  std::cout << "Success." << std::endl << std::endl;
}

template< std::size_t N = 1>
void TestCase3() {
  std::cout << "TEST CASE 2. Threads: " << N << std::endl;
  std::cout << "Info: Testing thread manager thread starvation/balancing" << std::endl;

  for (std::size_t i = 0; i < 10; ++i)
  {
    {
      ThreadManager tmanager(N);
      tmanager.Start();

      std::vector<int> ints{0,0,0,0};
      int testRunning = 0;

      for (std::size_t i = 0; i < ints.size(); ++i)
      {
        tmanager.Post([&ints, &testRunning, i]() {

            while(testRunning < 1) {}

            while(testRunning == 1)
            {
              ints[i]++;
              // Will never finish if this is commented out.
              std::this_thread::sleep_for(std::chrono::microseconds(1));
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
  std::cout << "Success." << std::endl << std::endl;
}

template< std::size_t N = 1>
void TestCase4() {
  std::cout << "TEST CASE 4. Threads: " << N << std::endl;
  std::cout << "Info: Stopping thread manager through its own post mechanism" << std::endl;
  for (std::size_t i = 0; i < 1000; ++i)
    {
      ThreadManager tmanager(N);
      tmanager.Start();
      tmanager.Post([&tmanager]() { tmanager.Stop(); });
    }
  std::cout << "Success." << std::endl << std::endl;
}

int main(int argc, char* argv[]) {

  TestCase1<1>();
  TestCase2<1>();
  TestCase3<1>();
  TestCase4<1>();

  TestCase1<10>();
  TestCase2<10>();
  TestCase3<10>();
  TestCase4<10>();

  std::cerr << "finished all tests" << std::endl;
  return 0;
}
