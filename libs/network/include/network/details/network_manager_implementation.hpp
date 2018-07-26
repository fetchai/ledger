#pragma once

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

class NetworkManagerImplementation : public std::enable_shared_from_this< NetworkManagerImplementation >  {
 public:
  typedef std::weak_ptr<NetworkManagerImplementation>   weak_ptr_type;
  typedef std::shared_ptr<NetworkManagerImplementation> shared_ptr_type;

  NetworkManagerImplementation(std::size_t threads = 1)
      : number_of_threads_(threads) {

    fetch::logger.Debug("Creating network manager");
  }

  ~NetworkManagerImplementation() {
    fetch::logger.Debug("Destroying network manager");
  }

  NetworkManagerImplementation(NetworkManagerImplementation const& ) = delete;
  NetworkManagerImplementation(NetworkManagerImplementation && ) = default ;

  void Start()
  {
    std::lock_guard<std::mutex> lock(thread_mutex_);

    if (threads_.size() == 0) {
      fetch::logger.Info("Starting network manager");
      {
        std::lock_guard< fetch::mutex::Mutex > lock( owning_mutex_ );
        owning_thread_ = std::this_thread::get_id();
      }

      shared_work_ = std::make_shared<asio::io_service::work>(*io_service_);

      shared_ptr_type self = shared_from_this();
      for (std::size_t i = 0; i < number_of_threads_; ++i) {
        threads_.push_back(new std::thread([this, self]() {
        io_service_->run();
            }));
      }
    }
  }

  void Stop()
  {
    std::lock_guard< std::mutex > lock( thread_mutex_ );

    {
      std::lock_guard< fetch::mutex::Mutex > lock( owning_mutex_ );
      if( std::this_thread::get_id() != owning_thread_ ) {
        fetch::logger.Warn("Same thread must start and stop NetworkManager.");
        return;
      }
    }

    if (threads_.size() != 0) {

      shared_work_.reset();
      fetch::logger.Info("Stopping network manager");

      io_service_->stop();

      for (auto &thread : threads_) {
        thread->join();
        delete thread;
      }

      threads_.clear();
      {
        std::lock_guard< mutex::Mutex > lock(protecting_io_);
        io_service_.reset(new asio::io_service);
      }      
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
    std::lock_guard< mutex::Mutex > lock(protecting_io_);      
    io_service_->post(std::move(f));
  }

 private:
  std::thread::id owning_thread_;
  std::size_t number_of_threads_ = 1;
  std::vector<std::thread *> threads_;
  std::unique_ptr<asio::io_service> io_service_{new asio::io_service};
  mutex::Mutex protecting_io_;

  std::shared_ptr<asio::io_service::work> shared_work_;

  mutable std::mutex          thread_mutex_{};
  mutable fetch::mutex::Mutex owning_mutex_{__LINE__, __FILE__};
};

}
}
}

