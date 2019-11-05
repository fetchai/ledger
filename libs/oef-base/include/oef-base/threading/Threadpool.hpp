#pragma once
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

#include <functional>
#include <iostream>
#include <thread>
#include <vector>

class Threadpool
{
public:
  using ThreadP = std::shared_ptr<std::thread>;
  using Threads = std::vector<ThreadP>;

  Threadpool()
  {}
  virtual ~Threadpool()
  {
    try
    {
      stop();
    }
    catch (std::exception &e)
    {
      std::cerr << " Exception while shuting down threads: " << e.what() << std::endl;
    }
  }

  void start(std::size_t threadcount, std::function<void(void)> runnable)
  {
    threads.reserve(threadcount);
    for (std::size_t thread_idx = 0; thread_idx < threadcount; ++thread_idx)
    {
      threads.emplace_back(std::make_shared<std::thread>(runnable));
    }
  }

  void start(std::size_t threadcount, std::function<void(std::size_t thread_number)> runnable)
  {
    threads.reserve(threadcount);
    for (std::size_t thread_number = 0; thread_number < threadcount; ++thread_number)
    {
      threads.emplace_back(
          std::make_shared<std::thread>([runnable, thread_number]() { runnable(thread_number); }));
    }
  }

  virtual void stop()
  {
    Threads tmp;
    std::swap(tmp, threads);
    for (auto &thread : tmp)
    {
      if (std::this_thread::get_id() != thread->get_id())
      {
        thread->join();
      }
    }
  }

protected:
private:
  Threads threads;

  Threadpool(const Threadpool &other) = delete;
  Threadpool &operator=(const Threadpool &other)  = delete;
  bool        operator==(const Threadpool &other) = delete;
  bool        operator<(const Threadpool &other)  = delete;
};
