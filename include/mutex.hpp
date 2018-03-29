#ifndef MUTEX_HPP
#define MUTEX_HPP

#include"logger.hpp"

#include<thread>
#include<mutex>
#include<map>
#include<iostream>
#include<memory>

namespace fetch 
{
namespace mutex 
{
class ProductionMutex : public AbstractMutex
{

public:
  ProductionMutex(int , std::string ) { } 
  ProductionMutex() = default;  
};
  
class DebugMutex : public AbstractMutex
{
  struct LockInfo
  {
    bool locked = true;
  };

  class MutexTimeout 
  {
  public:
    MutexTimeout(std::string const &filename, std::size_t const &line, double const timeout = 100000) :
      filename_(filename), line_(line)
    {
      LOG_STACK_TRACE_POINT;

      running_ = true;
      created_ =  std::chrono::system_clock::now();        
      thread_ = std::thread([=](){
          LOG_LAMBDA_STACK_TRACE_POINT;
          
          double ms = 0;
          
          while((running_)  && (ms < timeout)) {
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
            ms =  std::chrono::duration_cast<std::chrono::milliseconds>(end - created_).count();

          }
          if(running_)
          {            
            this->Eval();
          }
          
        });      
    }
    ~MutexTimeout() 
    {
      running_ = false;
      thread_.join();      
    }
    
    void Eval() 
    {
      LOG_STACK_TRACE_POINT;
      fetch::logger.Error( "Mutex timed out: ", filename_ ," ", line_ );
      
      exit(-1);      
    }
    
  private:
    std::string filename_;
    std::size_t line_;
    
    std::thread thread_;
    std::chrono::system_clock::time_point created_;
    std::atomic< bool > running_;

  };
   
    
public:
  DebugMutex(int line, std::string file) : AbstractMutex(),  line_(line), file_(file) { } 
  DebugMutex() = default;
  
    
  DebugMutex& operator=(DebugMutex const &other) = delete;
  
  void lock() 
  {
    LOG_STACK_TRACE_POINT;
    lock_mutex_.lock();    
    locked_ =   std::chrono::high_resolution_clock::now();
    lock_mutex_.unlock();
    
    std::mutex::lock();

    timeout_ =  std::unique_ptr<MutexTimeout>( new MutexTimeout(file_, line_) );
    fetch::logger.RegisterLock( this );
    thread_id_ = std::this_thread::get_id();     
  }
  
  void unlock() 
  {
    LOG_STACK_TRACE_POINT;

    lock_mutex_.lock();
    std::chrono::high_resolution_clock::time_point end_time = std::chrono::high_resolution_clock::now();
    double total_time =  std::chrono::duration_cast<std::chrono::milliseconds>(end_time - locked_).count();    
    lock_mutex_.unlock();    
    
    timeout_.reset(nullptr);     
    fetch::logger.RegisterUnlock( this, total_time, file_, line_ );

    
    
    std::mutex::unlock();

  }
  
  int line() const 
  {
    return line_;
  }
  
  std::string filename() const 
  {
    return file_;
  }

  std::string AsString() override
  {
    std::stringstream ss;    
    ss << "Locked by thread #" << fetch::log::ReadableThread::GetThreadID( thread_id_ )  << " in " << filename() << " on " << line() ;    
    return ss.str();
  }

  std::thread::id thread_id() const override
  {
    return thread_id_;
  }
  
private:
  std::mutex lock_mutex_;
  std::chrono::high_resolution_clock::time_point locked_;
  
  int line_ = 0;
  std::string file_ = "";
  std::unique_ptr< MutexTimeout > timeout_;
  std::thread::id thread_id_;
  
};



#ifndef NDEBUG
typedef DebugMutex Mutex;
#else
typedef ProductionMutex Mutex;
#endif
};
};

#endif
