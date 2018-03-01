#ifndef LOGGER_HPP
#define LOGGER_HPP
#include"abstract_mutex.hpp"
#include"commandline/vt100.hpp"
#include<iostream>
#include<iomanip>
#include <ctime>
#include<chrono>
#include<mutex>
#include<atomic>
#include<thread>
#include<unordered_map>
#include<unordered_set>
#include<vector>
namespace fetch {

namespace log {

class ReadableThread {
public:  
  static int GetThreadID(std::thread::id const &thread ) 
  {
    mutex_.lock();       
    
    if(thread_number_.find(thread) == thread_number_.end() )
    {
      thread_number_[thread] = int(++thread_count_);      
    }
    int thread_number = thread_number_[thread];
    mutex_.unlock();
    
    return thread_number;    
  }
  
private:    
  static std::map< std::thread::id, int > thread_number_;
  static int thread_count_;
  static std::mutex mutex_;
  
};

std::map< std::thread::id, int > ReadableThread::thread_number_ = std::map< std::thread::id, int > ();
int ReadableThread::thread_count_ = 0;
std::mutex ReadableThread::mutex_ ;

class ContextDetails 
{
public:
  typedef std::shared_ptr< ContextDetails > shared_type;
  ContextDetails() :
    context_("(root)"),
    filename_(""),
    line_(0)
  {
    id_ = std::this_thread::get_id();    
  }
  
  ContextDetails(shared_type ctx, shared_type parent, std::string const & context, std::string const & filename = "", std::size_t const &line = 0) :
    context_(context),
    filename_(filename),
    line_(line),
    parent_(parent),  
    derived_from_(ctx)
  {
    id_ = std::this_thread::get_id();    
  }
  
  ContextDetails(shared_type parent, std::string const & context, std::string const & filename = "", std::size_t const &line = 0) :
    context_(context),
    filename_(filename),
    line_(line),
    parent_(parent)
  {
    id_ = std::this_thread::get_id();        
  }
  
  ~ContextDetails() 
  {
  }
  
  shared_type parent() 
  {
    return parent_;
  }

  shared_type derived_from() 
  {
    return derived_from_;
  }
  
  std::string context() const { return context_; }  
  std::string filename() const { return filename_; }
  std::size_t line() const { return line_; }  

  std::thread::id thread_id() const { return id_; }  
private:
  std::string context_;  
  std::string filename_;
  std::size_t line_;
  shared_type parent_;
  shared_type derived_from_;
  std::thread::id id_;
  
};
  
class Context 
{
public:
  typedef std::shared_ptr< ContextDetails > shared_type;
  Context();    
  Context(shared_type ctx, std::string const & context, std::string const & filename = "", std::size_t const &line = 0);  
  Context(std::string const & context, std::string const & filename = "", std::size_t const &line = 0) ;

  Context(Context const &context) 
  {
    details_ = context.details_;
    primary_ = false;
  }
  
  
  Context const& operator=(Context const &context) = delete;
  
  ~Context();
  
  shared_type details() const 
  {
    return details_;    
  }
  
private:
  shared_type details_;
  bool primary_ = true;
  
};



class DefaultLogger 
{
public:
  typedef std::shared_ptr< ContextDetails > shared_context_type;
  
  DefaultLogger() 
  {
  }
  virtual ~DefaultLogger() 
  {

  }
  
    
  enum {
    ERROR = 0,
    WARNING = 1,    
    INFO = 2,
    DEBUG = 3
  };
  
  virtual void StartEntry( int type, shared_context_type ctx ) 
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

    int thread_number = ReadableThread::GetThreadID( std::this_thread::get_id() );    
    
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
    
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::cout << "[ " << GetColor(color,9) << std::put_time(std::localtime(&now_c), "%F %T") ;
    std::cout << "." << std::setw(3) <<millis <<  DefaultAttributes() << ", #" << thread_number;
    std::cout << ": " << std::setw(20) << ctx->context() <<  " ] ";    
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
private:

};

namespace details {
  
class LogWrapper 
{
public:
  typedef std::shared_ptr< ContextDetails > shared_context_type;
  
  LogWrapper() 
  {
    log_ = new DefaultLogger();    
  }


  template< typename ...Args >
  void Info(Args ... args) 
  {
    std::lock_guard< std::mutex > lock( mutex_ );
    this->log_->StartEntry(DefaultLogger::INFO, TopContextImpl() );    
    Unroll<Args...>::Append( this, args... );
    this->log_->CloseEntry(DefaultLogger::INFO); 
  }

  template< typename ...Args >
  void Warn(Args ... args) 
  {
    std::lock_guard< std::mutex > lock( mutex_ );
    this->log_->StartEntry(DefaultLogger::WARNING, TopContextImpl() );    
    Unroll<Args...>::Append( this, args... );
    this->log_->CloseEntry(DefaultLogger::WARNING); 
  }

  template< typename ...Args >
  void Error(Args ... args) 
  {
    
    std::lock_guard< std::mutex > lock( mutex_ );
    this->log_->StartEntry(DefaultLogger::ERROR, TopContextImpl() );    
    Unroll<Args...>::Append( this, args... );
    this->log_->CloseEntry(DefaultLogger::ERROR);

    StackTrace();    
  }
  
  template< typename ...Args >
  void Debug(Args ... args) 
  {

    std::lock_guard< std::mutex > lock( mutex_ );
    this->log_->StartEntry(DefaultLogger::DEBUG, TopContextImpl() );    
    Unroll<Args...>::Append( this, args... );
    this->log_->CloseEntry(DefaultLogger::DEBUG); 
  }

  void SetContext(shared_context_type ctx) 
  {
    std::thread::id id =  std::this_thread::get_id();    
    std::lock_guard< std::mutex > lock(mutex_);
    context_[id] = ctx;        
  }


  
  shared_context_type TopContext()
  {
    std::lock_guard< std::mutex > lock(mutex_);  
    return TopContextImpl();    
  }

  void RegisterLock(fetch::mutex::AbstractMutex *ptr )
  {
    std::lock_guard< std::mutex > lock(mutex_);
    active_locks_.insert( ptr );
    
  }

  void RegisterUnlock(fetch::mutex::AbstractMutex *ptr )
  {
    std::lock_guard< std::mutex > lock(mutex_);
    auto it = active_locks_.find( ptr );
    if( it != active_locks_.end() )
    {
      active_locks_.erase( it );      
    }
    
  }

  void StackTrace(shared_context_type ctx,
    uint32_t max = uint32_t(-1),
    bool show_locks = true, std::string const &trace_name = "Stack trace" ) 
  {
    if(!ctx) {
      std::cerr << "Stack trace context invalid" << std::endl;
      return;      
    }
    
    std::cout << trace_name << " for #" << ReadableThread::GetThreadID( ctx->thread_id() )  <<  std::endl;    
    PrintTrace(ctx, max);    

    if(show_locks) {
      
      std::vector< std::thread::id > locked_threads;
      
      std::cout << std::endl;    
      std::cout << "Active locks: " << std::endl;
      for(auto &l : active_locks_) {
        std::cout << "  - " << l->AsString() << std::endl;
        locked_threads.push_back( l->thread_id() );
        
      }
      std::cout << std::endl;
      for(auto &id: locked_threads) {
        std::cout << "Additionally trace for #" << ReadableThread::GetThreadID( id )  <<  std::endl;    
        ctx = context_[id];
        PrintTrace(ctx);
        std::cout << std::endl;      
      }
    }
  }
  

  void StackTrace(uint32_t max = uint32_t(-1), bool show_locks = true ) 
  {
    shared_context_type ctx = TopContextImpl();
    StackTrace(ctx, max, show_locks);    
  }

 
private:
  std::unordered_set< fetch::mutex::AbstractMutex * > active_locks_;
  
  shared_context_type TopContextImpl()
  {    
    std::thread::id id =  std::this_thread::get_id(); 
    if( !context_[id] ) {
      context_[id] = std::make_shared<ContextDetails>();      
    }
    
    return context_[id];    
  }
  
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


  void PrintTrace(shared_context_type ctx, uint32_t max = uint32_t(-1)) 
  {
    using namespace fetch::commandline::VT100;
    std::size_t i = 0;    
    while( ctx )
    {
      std::cout << std::setw(3) << i <<  ": In thread #" << ReadableThread::GetThreadID( ctx->thread_id() ) << ": ";      
      std::cout << GetColor(5,9) << ctx->context() << DefaultAttributes() << " " << ctx->filename() << ", ";
      std::cout << GetColor(3,9) << ctx->line() << DefaultAttributes() << std::endl;

      if( ctx->derived_from() ) {
        std::cout << "*";
        ctx = ctx->derived_from();        
      } else {
        ctx = ctx->parent();
      }
      ++i;
      if(i >= max) break;      
    }
  }
  
  
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
  mutable std::mutex mutex_;
  std::unordered_map< std::thread::id, shared_context_type > context_;
  
};
};

};

log::details::LogWrapper logger;  


namespace log {
Context::Context() 
{
  details_ = std::make_shared< ContextDetails >();
  fetch::logger.SetContext( details_ );
}

 
Context::Context(shared_type ctx,  std::string const & context, std::string const & filename, std::size_t const &line)
{
  details_ = std::make_shared< ContextDetails >(ctx, fetch::logger.TopContext(), context, filename, line);
  fetch::logger.SetContext( details_ );  
}

Context::Context(std::string const & context , std::string const & filename, std::size_t const &line ) 
{
  details_ = std::make_shared< ContextDetails >(fetch::logger.TopContext(), context, filename, line);    
  fetch::logger.SetContext( details_ );
}

Context::~Context() 
{
  if(primary_ && details_->parent() )
  {    
    fetch::logger.SetContext( details_->parent() );
  }
  
}


};
};

// TODO: Move somewhere else

#ifndef __FUNCTION_NAME__
    #ifdef WIN32   //WINDOWS
        #define __FUNCTION_NAME__   __FUNCTION__  
    #else          //*NIX
        #define __FUNCTION_NAME__   __func__ 
    #endif
#endif

#define LOG_STACK_TRACE_POINT \
  fetch::log::Context log_context(__FUNCTION_NAME__, __FILE__, __LINE__)

#define LOG_LAMBDA_STACK_TRACE_POINT \
  fetch::log::Context log_lambda_context(log_context.details(), __FUNCTION_NAME__, __FILE__, __LINE__)  

#define LOG_CONTEXT_VARIABLE(name) \
  std::shared_ptr< fetch::log::ContextDetails > name

#define LOG_SET_CONTEXT_VARIABLE(name) \
  name = fetch::logger.TopContext()  

#define LOG_PRINT_STACK_TRACE(name, custom_name)                    \
  fetch::logger.StackTrace( name, uint32_t(-1), false, custom_name )  
  

//#define LOG_STACK_TRACE_POINT
//#define LOG_LAMBDA_STACK_TRACE_POINT
#endif

