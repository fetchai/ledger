#ifndef NETWORK_THREAD_MANAGER_HPP
#define NETWORK_THREAD_MANAGER_HPP
#include "core/assert.hpp"
#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "network/details/thread_manager_implementation.hpp"

#include <functional>
#include <map>
#include "network/fetch_asio.hpp"

namespace fetch
{
namespace network
{

class ThreadManager
{
 public:
  typedef std::function<void()> event_function_type;

  typedef details::ThreadManagerImplementation            implementation_type;
  typedef typename implementation_type::event_handle_type event_handle_type;
  typedef std::shared_ptr<implementation_type>            pointer_type;
  typedef std::weak_ptr<implementation_type>              weak_ref_type;
  typedef implementation_type::shared_socket_type         shared_socket_type;
  typedef implementation_type::socket_type                socket_type;

  explicit ThreadManager(std::size_t threads = 1)
  {
    std::cout << "ThreadManager MAKING A REAL ONE." << threads << std::endl;
    pointer_      = std::make_shared< implementation_type >( threads );
  }

  ThreadManager(ThreadManager const &other) :
    is_copy_(true)
  {
    if(other.is_copy_)
    {
      weak_pointer_ = other.weak_pointer_;
    } else {
      weak_pointer_ = other.pointer_;
    }
  }

  ~ThreadManager()
  {
    Identify("ThreadManager::~ThreadManager.");
    if(!is_copy_)
    {
      Stop();
    }
    else
      {
      }
  }

  ThreadManager(ThreadManager &&rhs)                 = delete;
  ThreadManager &operator=(ThreadManager const &rhs)
  {
    is_copy_ = true;
    if(rhs.is_copy_)
      {
        weak_pointer_ = rhs.weak_pointer_;
      } else {
      weak_pointer_ = rhs.pointer_;
    }
    return *this;
  }

  ThreadManager &operator=(ThreadManager&& rhs)      = delete;

  void Identify(const char *prefix)
  {
    if (!this)
      {
        std::cout << prefix << "PTR: NULL" << std::endl;
      }
    else
      {
        std::cout << prefix << "PTR: " <<  this << "  COPY:" << is_copy_ << std::endl;
      }
  }

  void Start()
  {
    Identify("ThreadManager::Start");
    if(is_copy_) return;
    auto ptr = lock();
    if(ptr)
    {
      ptr->Start();
    }
  }

  void Stop()
  {
    Identify("ThreadManager::Stop");
    if(is_copy_)
    {
      return;
    }
    auto ptr = lock();
    if(ptr)
    {
      ptr->Stop();
    }
  }

  template <typename F>
  void Post(F &&f)
  {
    auto ptr = lock();
    if(ptr)
    {
      return ptr->Post(f);
    } else {
      fetch::logger.Info("Failed to post: thread man dead.");
    }
  }

  template <typename F>
  void Post(F &&f, const char *noisy)
  {
    auto ptr = lock();
    if(ptr)
    {
      return ptr->Post(f, noisy);
    } else {
      fetch::logger.Info("Failed to post: thread man dead.");
    }
  }

  template <typename F>
  void Post(F &&f, int milliseconds)
  {
    auto ptr = lock();
    if(ptr)
    {
      return ptr->Post(f, milliseconds);
    }
  }

  bool is_valid()
  {
    return (!is_copy_) || bool(weak_pointer_.lock());
  }

  bool is_primary()
  {
    return (!is_copy_);
  }

  pointer_type lock()
  {
    if(is_copy_)
    {
      return weak_pointer_.lock();
    }
    return pointer_;
  }

  template <typename IO, typename... arguments>
  std::shared_ptr<IO> CreateIO(arguments&&... args)
  {
    auto ptr = lock();
    if(ptr)
    {
      return ptr->CreateIO<IO>(std::forward<arguments>(args)...);
    }
    std::cout << "Attempted to get IO from dead TM" << std::endl;
    return std::shared_ptr<IO>(nullptr);
  }

  // TODO: (`HUT`) : delete this
  asio::io_service &io_service() { return pointer_->io_service(); }

 private:
  pointer_type pointer_;
  weak_ref_type weak_pointer_;
  bool is_copy_ = false;
};
}
}

#endif
