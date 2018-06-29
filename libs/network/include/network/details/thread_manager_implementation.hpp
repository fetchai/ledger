#ifndef NETWORK_DETAILS_THREAD_MANAGER_IMPLEMENTATION_HPP
#define NETWORK_DETAILS_THREAD_MANAGER_IMPLEMENTATION_HPP
#include "core/assert.hpp"
#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "network/fetch_asio.hpp"

#include <functional>
#include <map>
#include <memory>

namespace fetch {
namespace network {
namespace details {

class ThreadManagerImplementation : public std::enable_shared_from_this< ThreadManagerImplementation >  {
 public:
  typedef std::function<void()>                        event_function_type; // TODO: (`HUT`) : curate
  typedef uint64_t                                     event_handle_type;
  typedef std::weak_ptr<ThreadManagerImplementation>   weak_ptr_type;
  typedef std::shared_ptr<ThreadManagerImplementation> shared_ptr_type;
  typedef std::shared_ptr<asio::ip::tcp::tcp::socket>  shared_socket_type;
  typedef asio::ip::tcp::tcp::socket                   socket_type;


  ThreadManagerImplementation(std::size_t threads = 1)
      : number_of_threads_(threads) {
    started_flag_ = false;
      
    fetch::logger.Debug("Creating thread manager");
  }

  ~ThreadManagerImplementation() {
    fetch::logger.Debug("Destroying thread manager");
  }

  ThreadManagerImplementation(ThreadManagerImplementation const& ) = delete;
  ThreadManagerImplementation(ThreadManagerImplementation && ) = default ;

  void Start()
  {
    std::lock_guard< fetch::mutex::Mutex > lock( thread_mutex_ );

    if (threads_.size() == 0) {
      fetch::logger.Info("Starting thread manager");
      {
        std::lock_guard< fetch::mutex::Mutex > lock( owning_mutex_ );
        owning_thread_ = std::this_thread::get_id();
      }

      shared_work_ = std::make_shared<asio::io_service::work>(*io_service_);

      // TODO: (`HUT`) : look at this code, might be a race
      shared_ptr_type self = shared_from_this();

      started_flag_ = true;
      
      for (std::size_t i = 0; i < number_of_threads_; ++i) {
        threads_.push_back(new std::thread([this, self]() {
              io_service_->run();
            }));
      }
    }
  }

  void Stop()
  {
    std::lock_guard< fetch::mutex::Mutex > lock( thread_mutex_ );

    {
      std::lock_guard< fetch::mutex::Mutex > lock( owning_mutex_ );
      if( std::this_thread::get_id() != owning_thread_ ) {
        fetch::logger.Warn("Same thread must start and stop ThreadManager.");
        return;
      }
    }

    if (threads_.size() != 0) {

      shared_work_.reset();
      fetch::logger.Info("Stopping thread manager");

      io_service_->stop();

      for (auto &thread : threads_) {
        thread->join();
        delete thread;
      }

      threads_.clear();
      protecting_io_ = true;
      io_service_.reset(new asio::io_service);
      protecting_io_ = false;
    }
  }

  // Must only be called within a post, then the io_service_ is always guaranteed to be valid
  template <typename IO, typename... arguments>
  std::shared_ptr<IO> CreateIO(arguments&&... args)
  {
    return std::make_shared<IO>(*io_service_, std::forward<arguments>(args)...);
  }

  template <typename F>
  void Post(F &&f)
  {
    std::cout << "POST " << started_flag_ << std::endl;
    if(!protecting_io_)
    {
      io_service_->post(std::move(f));
      thread_mutex_.unlock();
    } else {
      fetch::logger.Info("Failed to post: io_service protected.");
    }
  }

 private:

  bool started_flag_;
  
  std::thread::id owning_thread_;
  std::size_t number_of_threads_ = 1;
  std::vector<std::thread *> threads_;
  std::unique_ptr<asio::io_service> io_service_{new asio::io_service};
  bool protecting_io_{false};

  std::shared_ptr<asio::io_service::work> shared_work_;

  mutable fetch::mutex::Mutex thread_mutex_{__LINE__, __FILE__};
  mutable fetch::mutex::Mutex owning_mutex_{__LINE__, __FILE__};
};

}
}
}

#endif
