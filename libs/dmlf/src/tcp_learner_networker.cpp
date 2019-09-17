//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "dmlf/tcp_learner_networker.hpp"
#include "dmlf/update.hpp"

#include "network/service/function.hpp"

#include <random>

namespace fetch {
namespace dmlf {

uint16_t ephem_port_()
{
  uint16_t                                from = 50000;
  uint16_t                                to   = 65000;
  std::random_device                      rand_dev;
  std::mt19937                            generator(rand_dev());
  std::uniform_int_distribution<uint16_t> distr(from, to);
  return distr(generator);
}

// void TcpLearnerNetworker::start()
void TcpLearnerNetworker::start_()
{
  if (nm_mine_)
    nm_->Start();

  server_->Start();

  for (auto &upd_in : clients_)
  {
    while (!upd_in->is_alive())
    {
      std::cout << "Waiting for client to connect" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}

void TcpLearnerNetworker::pushUpdate(std::shared_ptr<IUpdate> update)
{
  broadcast_update_(update);
}

std::size_t TcpLearnerNetworker::getUpdateCount() const
{
  Lock lock{updates_m_};
  // return updates_.size();
  return updates_bytes_.size();
}

TcpLearnerNetworker::Intermediate TcpLearnerNetworker::getUpdateIntermediate()
{
  Lock lock{updates_m_};
  /*
  if(!updates_.empty())
  {
    std::shared_ptr<IUpdate> upd = updates_.top();
    updates_.pop();
    return upd;
  }
  return std::shared_ptr<IUpdate>{nullptr};
  */
  if (!updates_bytes_.empty())
  {
    Intermediate upd = updates_bytes_.front();
    updates_bytes_.pop_front();
    return upd;
  }
  throw std::length_error{"Updates queue is empty"};
  return Intermediate{};
}

std::size_t TcpLearnerNetworker::getPeerCount() const
{
  return clients_.size();
}

}  // namespace dmlf
}  // namespace fetch
