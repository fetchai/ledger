#include<iostream>
#include<thread>
#include<memory>
#include<atomic>
#include"core/logger.hpp"
#include"core/mutex.hpp"

fetch::mutex::Mutex mt;
std::unique_ptr< std::thread > thread;
std::atomic<int> n ;

void Foo();


void Baz() 
{
  LOG_STACK_TRACE_POINT;
  std::cout << "Baz" << std::endl;  

  if( n >= 2) {
    fetch::logger.Error("XX");
        
    exit(-1);
            
  }

  std::lock_guard< fetch::mutex::Mutex > lock(mt);
  
  thread.reset( new std::thread([=]() {
        LOG_LAMBDA_STACK_TRACE_POINT;
        ERROR_BACKTRACE;
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
