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

#include "core/serializers/base_types.hpp"
#include "dmlf/deprecated/local_learner_networker.hpp"
#include "dmlf/deprecated/update_interface.hpp"

#include <iostream>

namespace fetch {
namespace dmlf {

void deprecated_LocalLearnerNetworker::AddPeers(
    std::vector<std::shared_ptr<deprecated_LocalLearnerNetworker>> new_peers)
{
  for (auto const &peer : new_peers)
  {
    if (peer.get() != this)
    {
      peers_.emplace_back(peer);
    }
  }
}

void deprecated_LocalLearnerNetworker::ClearPeers()
{
  peers_.clear();
}

void deprecated_LocalLearnerNetworker::PushUpdate(deprecated_UpdateInterfacePtr const &update)
{
  std::vector<std::shared_ptr<deprecated_LocalLearnerNetworker>> targets;

  auto indexes = alg_->GetNextOutputs();
  auto data    = update->Serialise();

  for (auto const &ind : indexes)
  {
    auto t = peers_[ind];
    t->Recieve(data);
  }
}

void deprecated_LocalLearnerNetworker::Recieve(Bytes const &data)
{
  // since we're in the same process, there's no processing to do to
  // the message, we can simply deliver it.
  NewMessage(data);
}

}  // namespace dmlf
}  // namespace fetch
