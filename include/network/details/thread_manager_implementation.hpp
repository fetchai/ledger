#ifndef NETWORK_DETAILS_THREAD_MANAGER_IMPLEMENTATION_HPP
#define NETWORK_DETAILS_THREAD_MANAGER_IMPLEMENTATION_HPP
#include "assert.hpp"
#include "logger.hpp"
#include "mutex.hpp"

#include <functional>
#include <map>
#include "fetch_asio.hpp"

namespace fetch {
namespace network {
namespace details {

class ThreadManagerImplementation {
 public:
  typedef std::function<void()> event_function_type;
  typedef uint64_t event_handle_type;

  ThreadManagerImplementation(std::size_t threads = 1)
      : number_of_threads_(threads), on_mutex_(__LINE__, __FILE__) {
    fetch::logger.Debug("Creating thread manager");
    running_ = false;
  }

  ~ThreadManagerImplementation() {
    fetch::logger.Debug("Destroying thread manager");
    Stop();
  }

  void Start() {
    std::lock_guard< fetch::mutex::Mutex > lock( on_mutex_ );
    if (threads_.size() == 0) {
      fetch::logger.Info("Starting thread manager");
      running_ = true;

      shared_work_ = std::make_shared<asio::io_service::work>(io_service_);

      for (auto &obj : on_before_start_) {
        obj.second();
      }

      for (std::size_t i = 0; i < number_of_threads_; ++i) {
        threads_.push_back(new std::thread([this]() { io_service_.run(); }));
      }

      for (auto &obj : on_after_start_) {
        obj.second();
      }
    }
  }

  void Stop() {
    std::lock_guard< fetch::mutex::Mutex > lock( on_mutex_ );
    if (threads_.size() != 0) {
      shared_work_.reset();

      fetch::logger.Info("Stopping thread manager");
      for (auto &obj : on_before_stop_) {
        obj.second();
      }

      io_service_.stop();

      for (auto &thread : threads_) {
        thread->join();
        delete thread;
      }

      threads_.clear();

      for (auto &obj : on_after_stop_) {
        obj.second();
      }
    }
  }

  asio::io_service &io_service() { return io_service_; }

  event_handle_type OnBeforeStart(event_function_type const &fnc) {
    std::lock_guard< fetch::mutex::Mutex > lock( on_mutex_ );
    fetch::logger.Debug("Adding BeforeStart event listener ", next_id_,
                        " from thread manager ");
    on_before_start_[next_id_] = fnc;
    return next_id_++;
  }

  event_handle_type OnAfterStart(event_function_type const &fnc) {
    std::lock_guard< fetch::mutex::Mutex > lock( on_mutex_ );
    fetch::logger.Debug("Adding AfterStart event listener ", next_id_,
                        " from thread manager ");
    on_after_start_[next_id_] = fnc;
    return next_id_++;
  }

  event_handle_type OnBeforeStop(event_function_type const &fnc) {
    std::lock_guard< fetch::mutex::Mutex > lock( on_mutex_ );
    fetch::logger.Debug("Adding BeforeStop event listener ", next_id_,
                        " from thread manager ");
    on_before_stop_[next_id_] = fnc;
    return next_id_++;
  }

  event_handle_type OnAfterStop(event_function_type const &fnc) {
    std::lock_guard< fetch::mutex::Mutex > lock( on_mutex_ );
    fetch::logger.Debug("Adding AfterStop event listener ", next_id_,
                        " from thread manager ");
    on_after_stop_[next_id_] = fnc;
    return next_id_++;
  }

  void Off(event_handle_type handle) {
    std::lock_guard< fetch::mutex::Mutex > lock( on_mutex_ );
    fetch::logger.Debug("Removing event listener ", handle,
                        " from thread manager ");

    if (on_before_start_.find(handle) != on_before_start_.end()) {
      fetch::logger.Debug("Erasing BeforeStart.");
      on_before_start_.erase(handle);
    }

    if (on_after_start_.find(handle) != on_after_start_.end()) {
      fetch::logger.Debug("Erasing AfterStart.");
      on_after_start_.erase(handle);
    }

    if (on_before_stop_.find(handle) != on_before_stop_.end()) {
      fetch::logger.Debug("Erasing BeforeStop.");
      on_before_stop_.erase(handle);
    }

    if (on_after_stop_.find(handle) != on_after_stop_.end()) {
      fetch::logger.Debug("Erasing AfterStop.");
      on_after_stop_.erase(handle);
    }
    fetch::logger.Debug("Done removing event listener ", handle,
                        " from thread manager ");
  }

  template <typename F>
  void Post(F &&f) {
    io_service_.post(std::move(f));
  }

  template <typename F>
  void Post(F &&f, int milliseconds) {
    // TODO: make class for delayed post such that we don't block the thread

    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    io_service_.post(std::move(f));
  }

 private:
  std::size_t number_of_threads_ = 1;
  std::vector<std::thread *> threads_;
  asio::io_service io_service_;
  std::atomic<bool> running_;
  std::shared_ptr<asio::io_service::work> shared_work_;

  std::map<event_handle_type, event_function_type> on_before_start_;
  std::map<event_handle_type, event_function_type> on_after_start_;

  std::map<event_handle_type, event_function_type> on_before_stop_;
  std::map<event_handle_type, event_function_type> on_after_stop_;
  event_handle_type next_id_ = 0;
  mutable fetch::mutex::Mutex on_mutex_;
};

}
}
}

#endif
