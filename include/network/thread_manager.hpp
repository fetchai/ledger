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


  ThreadManager(std::size_t threads = 1) {
    pointer_ = std::make_shared< implementation_type >( threads );
  }

  ~ThreadManager() {
  }

  ThreadManager(ThreadManager const &rhs)            = default;
  ThreadManager(ThreadManager &&rhs)                 = default;
  ThreadManager &operator=(ThreadManager const &rhs) = default;
  ThreadManager &operator=(ThreadManager&& rhs)      = default;

  void Start() {
    pointer_->Start();
  }

  void Stop() {
    pointer_->Stop();
  }

  asio::io_service &io_service() { return pointer_->io_service(); }

  event_handle_type OnBeforeStart(event_function_type const &fnc) {
    return pointer_->OnBeforeStart(fnc);
  }

  event_handle_type OnAfterStart(event_function_type const &fnc) {
    return pointer_->OnAfterStart(fnc);
  }

  event_handle_type OnBeforeStop(event_function_type const &fnc) {
    return pointer_->OnBeforeStop(fnc);
  }

  event_handle_type OnAfterStop(event_function_type const &fnc) {
    return pointer_->OnAfterStop(fnc);
  }

  void Off(event_handle_type handle) {
    pointer_->Off(handle);
  }

  template <typename F>
  void Post(F &&f) {
    pointer_->Post(std::move(f));
  }

  template <typename F>
  void Post(F &&f, int milliseconds) {
    pointer_->Post(std::move(f), milliseconds);
  }

 private:
  pointer_type pointer_;
};
}
}

#endif
