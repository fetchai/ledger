#include "core/logger.hpp"
#include "core/mutex.hpp"
#include <atomic>
#include <iostream>
#include <memory>
#include <thread>

fetch::mutex::Mutex          mt;
std::atomic<int>             n;

void Foo();

void Baz()
{
  LOG_STACK_TRACE_POINT;
  std::cout << "Baz" << std::endl;

  if (n >= 2)
  {
    fetch::logger.Error("XX");

    exit(-1);
  }

  std::lock_guard<fetch::mutex::Mutex> lock(mt);
  std::unique_ptr<std::thread> thread;
  
  thread.reset(new std::thread([=]() {
    LOG_LAMBDA_STACK_TRACE_POINT;

    Foo();
  }));
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
