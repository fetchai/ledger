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

#include "core/logger.hpp"
#include "core/mutex.hpp"

#include <atomic>
#include <iostream>
#include <memory>
#include <thread>
#include <utility>

static constexpr char const *LOGGING_NAME = "main";

fetch::mutex::Mutex mt;
std::atomic<int>    n;

void Foo();

void Baz()
{
  LOG_STACK_TRACE_POINT;
  std::cout << "Baz" << std::endl;

  if (n >= 2)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "XX");

    exit(-1);
  }

  std::lock_guard<fetch::mutex::Mutex> lock(mt);
  auto                                 thread = std::make_unique<std::thread>([=]() {
    LOG_LAMBDA_STACK_TRACE_POINT;
    Foo();
  });

  thread->join();
}

void Bar()
{
  LOG_STACK_TRACE_POINT;

  std::cout << "Bar" << std::endl;

  Baz();
}

void Foo()
{
  LOG_STACK_TRACE_POINT;

  std::cout << "Foo" << std::endl;

  ++n;
  Bar();
}

int main()
{
  n = 0;

  Foo();

  return 0;
}
