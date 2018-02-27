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
class ProductionMutex : public std::mutex 
{

public:
  ProductionMutex(int , std::string ) { } 
  ProductionMutex() = default;  
};
  
class DebugMutex : public std::mutex 
{
  struct LockInfo 
  {
    bool locked = true;
  };

  class MutexTimeout 
  {
  public:
    MutexTimeout(double const timeout = 2000)       
    {
      running_ = true;
      created_ =  std::chrono::system_clock::now();        
      thread_ = std::thread([this,timeout](){
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
      std::cerr << "Mutex timed out" << std::endl;
      exit(-1);      
    }
    
  private:
    std::thread thread_;
    std::chrono::system_clock::time_point created_;
    std::atomic< bool > running_;
    
  };
   
    
public:
  DebugMutex(int line, std::string file) : std::mutex(),  line_(line), file_(file) { } 
  DebugMutex() = default;
  
  
  
  DebugMutex& operator=(DebugMutex const &other) = delete;
  
  void lock() 
  {
        
    lock_mutex_.lock();

    std::thread::id id =  std::this_thread::get_id();
    if(locker_.find( id ) != locker_.end() ) 
    {
      fetch::logger.Debug( "Mutex deadlock: ", line_ , " " , file_ );            
      exit(-1);
    }
    locker_[id] = LockInfo();
    timeout_ =  std::unique_ptr<MutexTimeout>( new MutexTimeout() );
    lock_mutex_.unlock();
    std::mutex::lock();
  }
  
  void unlock() 
  {
    lock_mutex_.lock();
    timeout_.reset(nullptr);     
    std::thread::id id =  std::this_thread::get_id();
    locker_.erase( id );
    lock_mutex_.unlock();
    std::mutex::unlock();    
  }
private:
  std::map< std::thread::id, LockInfo > locker_;
  std::mutex lock_mutex_;
  int line_ = 0;
  std::string file_ = "";
  std::unique_ptr< MutexTimeout > timeout_;  
};


typedef DebugMutex Mutex;
};
};

#endif
