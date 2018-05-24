#ifndef NETWORK_THREAD_MANAGER_HPP
#define NETWORK_THREAD_MANAGER_HPP
#include "assert.hpp"
#include "logger.hpp"
#include "mutex.hpp"
#include "network/details/thread_manager_implementation.hpp"

#include <functional>
#include <map>
#include "fetch_asio.hpp"

namespace fetch {
namespace network {

class ThreadManager {
 public:
  typedef std::function<void()> event_function_type;

  typedef details::ThreadManagerImplementation implementation_type;
  typedef typename implementation_type::event_handle_type event_handle_type;
  typedef std::shared_ptr< implementation_type > pointer_type;
  typedef std::weak_ptr< implementation_type > weak_ref_type;  


  ThreadManager(std::size_t threads = 1) {
    pointer_ = std::make_shared< implementation_type >( threads );
  }

  ThreadManager(ThreadManager const &other) :
    is_copy_(true) {

    weak_pointer_ = other.pointer_;
  }
    
  ~ThreadManager() {
    if(!is_copy_) Stop();
  }

  ThreadManager(ThreadManager &&rhs)                 = default;
  ThreadManager &operator=(ThreadManager const &rhs) = delete;
  ThreadManager &operator=(ThreadManager&& rhs)      = delete;

  void Start() {
    if(is_copy_) return;
    auto ptr = lock();
    if(ptr)
      ptr->Start(); 
  }

  void Stop() {
    if(is_copy_) {
      std::cout << "Is copy !! exiting" << std::endl;
      return;
    }
    auto ptr = lock();
    if(ptr)
      ptr->Stop();
  }

  asio::io_service &io_service() {
    auto ptr = lock();
    return ptr->io_service();
  }

  event_handle_type OnBeforeStart(event_function_type const &fnc) {
    auto ptr = lock();
    if(ptr) {
      return ptr->OnBeforeStart(fnc);
    }    
    return event_handle_type(-1);
  }

  event_handle_type OnAfterStart(event_function_type const &fnc) {
    auto ptr = lock();
    if(ptr) {
      return ptr->OnAfterStart(fnc);
    }
    return event_handle_type(-1);
  }

  event_handle_type OnBeforeStop(event_function_type const &fnc) {
    auto ptr = lock();
    if(ptr) {
      return ptr->OnBeforeStart(fnc);
    }
    return event_handle_type(-1);
  }

  event_handle_type OnAfterStop(event_function_type const &fnc) {
    auto ptr = lock();
    if(ptr) {
      return ptr->OnAfterStop(fnc);
    }
    return event_handle_type(-1);
  }

  void Off(event_handle_type handle) {
    auto ptr = lock();
    if(ptr) {
      ptr->Off(handle);
    }

  }

  template <typename F>
  void Post(F &&f) {
    auto ptr = lock();
    if(ptr) {
      return ptr->Post(f);
    }
  }

  template <typename F>
  void Post(F &&f, int milliseconds) {
    auto ptr = lock();
    if(ptr) {
      return ptr->Post(f, milliseconds);
    }
  }

  bool is_valid() {
    return (!is_copy_) || bool(weak_pointer_.lock());
  }

  bool is_primary() {
    return (!is_copy_);
  }
  
  pointer_type lock() {
    if(is_copy_) {
      return weak_pointer_.lock();
    }
    return pointer_;
  }  
 private:

  pointer_type pointer_;
  weak_ref_type weak_pointer_;
  bool is_copy_ = false;
};
}
}

#endif
