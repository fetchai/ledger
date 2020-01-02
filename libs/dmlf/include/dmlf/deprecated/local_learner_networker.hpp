#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/mutex.hpp"
#include "core/serializers/base_types.hpp"
#include "dmlf/deprecated/abstract_learner_networker.hpp"
#include <list>
#include <map>
#include <memory>

namespace fetch {
namespace dmlf {

class deprecated_LocalLearnerNetworker : public deprecated_AbstractLearnerNetworker
{
public:
  using PeerP = std::shared_ptr<deprecated_LocalLearnerNetworker>;
  using Peers = std::vector<PeerP>;

  deprecated_LocalLearnerNetworker()                                              = default;
  ~deprecated_LocalLearnerNetworker() override                                    = default;
  deprecated_LocalLearnerNetworker(deprecated_LocalLearnerNetworker const &other) = delete;
  deprecated_LocalLearnerNetworker &operator=(deprecated_LocalLearnerNetworker const &other) =
      delete;
  bool operator==(deprecated_LocalLearnerNetworker const &other) = delete;
  bool operator<(deprecated_LocalLearnerNetworker const &other)  = delete;

  void PushUpdate(deprecated_UpdateInterfacePtr const &update) override;

  std::size_t GetPeerCount() const override
  {
    return peers_.size();
  }
  void AddPeers(Peers new_peers);
  void ClearPeers();

protected:
private:
  using Mutex = fetch::Mutex;
  using Lock  = std::unique_lock<Mutex>;
  using Bytes = deprecated_AbstractLearnerNetworker::Bytes;

  mutable Mutex mutex_;
  Peers         peers_;

  void Recieve(Bytes const &data);
};

}  // namespace dmlf
}  // namespace fetch
