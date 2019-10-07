#pragma once

#include <thread>
#include <iostream>
#include <functional>
#include <vector>

class Threadpool
{
public:
  using ThreadP  = std::shared_ptr<std::thread>;
  using Threads = std::vector<ThreadP>;
  
  Threadpool()
  {
  }
  virtual ~Threadpool()
  {
    try {
      stop();
    } catch (std::exception& e) {
      std::cerr << " Exception while shuting down threads: " << e.what() << std::endl;
    }
  }

  void start(std::size_t threadcount, std::function<void (void)> runnable)
  {
    threads.reserve(threadcount);
    for (std::size_t thread_idx = 0; thread_idx < threadcount; ++thread_idx)
    {
      threads.emplace_back(std::make_shared<std::thread>(runnable));
    }
  }

  void start(std::size_t threadcount, std::function<void (std::size_t thread_number)> runnable)
  {
    threads.reserve(threadcount);
    for (std::size_t thread_number = 0; thread_number < threadcount; ++thread_number)
    {
      threads.emplace_back(std::make_shared<std::thread>(
          [runnable,thread_number]() { runnable(thread_number); }
      ));
    }
  }

  virtual void stop()
  {
    for (auto &thread : threads)
    {
      if (std::this_thread::get_id() != thread->get_id())
      {
        thread->join();
      }
    }
    threads.empty();
  }

  
protected:
  

private:
  
  Threads threads;

  Threadpool(const Threadpool &other) = delete;
  Threadpool &operator=(const Threadpool &other) = delete;
  bool operator==(const Threadpool &other) = delete;
  bool operator<(const Threadpool &other) = delete;
};
