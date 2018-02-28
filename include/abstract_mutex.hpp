#ifndef ABSTRACT_MUTEX_HPP
#define ABSTRACT_MUTEX_HPP


#include<mutex>
#include <thread>

namespace fetch 
{
namespace mutex 
{

class AbstractMutex : public std::mutex  
{
public:
  AbstractMutex() : std::mutex() 
  {  }

  virtual std::string AsString() 
  {
    return "(mutex)";
  }

  virtual std::thread::id thread_id() const 
  {
    return std::thread::id();    
  }
  
};


};
};

#endif
