#ifndef MUTEX_HPP
#define MUTEX_HPP

#include<thread>
#include<mutex>
#include<map>
#include<iostream>

namespace fetch {
namespace mutex {
class ProductionMutex : public std::mutex {

public:
  ProductionMutex(int , std::string ) { } 
  ProductionMutex() = default;  
};
  
class DebugMutex : public std::mutex {
  struct LockInfo {
    bool locked = true;
  };
public:
  DebugMutex(int line, std::string file) : line_(line), file_(file) { } 
  DebugMutex() = default;
  
  
  DebugMutex& operator=(DebugMutex const &other) = delete;
  
  void lock() {
    lock_mutex_.lock();
    
    std::thread::id id =  std::this_thread::get_id();
    if(locker_.find( id ) != locker_.end() ) {
      std::cout << "MUTEX DEAD LOCK!: " << line_ << " " << file_ << std::endl << std::flush;
      exit(-1);
    }
    locker_[id] = LockInfo();
    
    lock_mutex_.unlock();


    std::mutex::lock();
  }
  
  void unlock() {
    lock_mutex_.lock();
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
};


typedef DebugMutex Mutex;
};
};

#endif
