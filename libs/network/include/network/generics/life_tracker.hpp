#ifndef LIFE_TRACKER_HPP
#define LIFE_TRACKER_HPP

#include <iostream>
#include <string>

namespace fetch {
namespace generics {
template <typename WORKER>
class LifeTracker
{
  using p_target_type = std::mutex;
  using strong_p_type = std::shared_ptr<p_target_type>;
  using weak_p_type   = std::weak_ptr<p_target_type>;
  using mutex_type    = std::mutex;
  using lock_type     = std::lock_guard<mutex_type>;

public:
  LifeTracker(fetch::network::NetworkManager worker) : worker_(worker) {}

  void reset(void)
  {
    lock_type lock(*alive_);
    auto      alsoAlive = alive_;
    alive_.reset();
  }

  void Post(std::function<void(void)> func)
  {
    std::weak_ptr<std::mutex> deadOrAlive(alive_);
    auto                      cb = [deadOrAlive, func]() {
      auto aliveOrElse = deadOrAlive.lock();
      if (aliveOrElse)
      {
        lock_type lock(*aliveOrElse);
        func();
      }
    };
    worker_.Post(func);
  }

private:
  strong_p_type                  alive_ = std::make_shared<p_target_type>();
  fetch::network::NetworkManager worker_;
};

}  // namespace generics
}  // namespace fetch

#endif  // LIFE_TRACKER_HPP
