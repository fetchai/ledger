#ifndef LOGGER_HPP
#define LOGGER_HPP
#include"commandline/vt100.hpp"
#include<iostream>
#include<iomanip>
#include <ctime>
#include<chrono>
#include<mutex>
namespace fetch {
namespace log {

class DefaultLogger 
{
public:
  enum {
    ERROR = 0,
    WARNING = 1,    
    INFO = 2,
    DEBUG = 3
  };
  
  virtual void StartEntry( int type ) 
  {
    using namespace fetch::commandline::VT100;
    int color = 9;
    switch(type) {
    case INFO:
      color = 3;
      break;
    case WARNING:
      color = 6;
      break;      
    case ERROR:
      color = 1;
      break;
    case DEBUG:
      color = 7;
      break;          
    }
    
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::cout << "[ " << GetColor(color,9) << std::put_time(std::localtime(&now_c), "%F %T") <<  DefaultAttributes() <<" ] ";    
  }

  template< typename T >
  void Append( T const &v ) 
  {
    std::cout << v ;    
  }
    
    
  virtual void Append( std::string const &s ) 
  {
    std::cout << s ;    
  }

  virtual void CloseEntry( int type ) 
  {
    std::cout << std::endl;    
  }
  
};

namespace details {
  
class LogWrapper 
{
public:
  LogWrapper() 
  {
    log_ = new DefaultLogger();    
  }

  template< typename ...Args >
  void Info(Args ... args) 
  {
    std::lock_guard< std::mutex > lock( mutex_ );
    this->log_->StartEntry(DefaultLogger::INFO);    
    Unroll<Args...>::Append( this, args... );
    this->log_->CloseEntry(DefaultLogger::INFO); 
  }

  template< typename ...Args >
  void Warn(Args ... args) 
  {
    std::lock_guard< std::mutex > lock( mutex_ );
    this->log_->StartEntry(DefaultLogger::WARNING);    
    Unroll<Args...>::Append( this, args... );
    this->log_->CloseEntry(DefaultLogger::WARNING); 
  }

  template< typename ...Args >
  void Error(Args ... args) 
  {
    
    std::lock_guard< std::mutex > lock( mutex_ );
    this->log_->StartEntry(DefaultLogger::ERROR);    
    Unroll<Args...>::Append( this, args... );
    this->log_->CloseEntry(DefaultLogger::ERROR); 
  }
  
  template< typename ...Args >
  void Debug(Args ... args) 
  {

    std::lock_guard< std::mutex > lock( mutex_ );
    this->log_->StartEntry(DefaultLogger::DEBUG);    
    Unroll<Args...>::Append( this, args... );
    this->log_->CloseEntry(DefaultLogger::DEBUG); 
  }
  
 
private:
  template< typename T, typename ... Args >
  struct Unroll 
  {
    static void Append(LogWrapper *cls, T const &v, Args ... args ) 
    {
      cls->log_->Append(v);
      Unroll< Args... >::Append(cls, args ...);
    }    
  };

  template< typename T >
  struct Unroll< T >
  {
    static void Append(LogWrapper *cls, T const &v ) 
    {
      cls->log_->Append(v);
    }    
  };

  /*
  template<  >
  struct Unroll<  >
  {
    static void Append(LogWrapper *cls ) 
    {
    }    
  };
  */  
  DefaultLogger *log_;
  std::mutex mutex_;
  
};
};

};

log::details::LogWrapper logger;  

};


#endif
