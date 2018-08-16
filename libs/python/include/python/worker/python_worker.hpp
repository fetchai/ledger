
#include "network/generics/network_node_core.hpp"
#include "network/swarm/swarm_agent_api_impl.hpp"
#include "network/swarm/swarm_http_interface.hpp"
#include "network/swarm/swarm_random.hpp"

#include <string>
#include <time.h>
#include <unistd.h>

namespace fetch {
namespace swarm {

class PythonWorker
{
public:
  using mutex_type = std::recursive_mutex;
  using lock_type  = std::lock_guard<std::recursive_mutex>;

  virtual void Start()
  {
    lock_type lock(mutex_);
    tm_->Start();
  }

  virtual void Stop()
  {
    lock_type lock(mutex_);
    tm_->Stop();
  }

  void UseCore(std::shared_ptr<fetch::network::NetworkNodeCore> nn_core) { nn_core_ = nn_core; }

  template <typename F>
  void Post(F &&f)
  {
    tm_->Post(f);
  }
  template <typename F>
  void Post(F &&f, uint32_t milliseconds)
  {
    tm_->Post(f, milliseconds);
  }

  PythonWorker() { tm_ = fetch::network::MakeThreadPool(1); }

  virtual ~PythonWorker() { Stop(); }

private:
  mutex_type                                       mutex_;
  fetch::network::ThreadPool                       tm_;
  std::shared_ptr<fetch::network::NetworkNodeCore> nn_core_;
};

}  // namespace swarm
}  // namespace fetch
