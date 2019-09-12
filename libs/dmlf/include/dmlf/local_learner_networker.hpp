#pragma once
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

#include "core/mutex.hpp"
#include "core/serializers/base_types.hpp"
#include "dmlf/ilearner_networker.hpp"
#include <list>
#include <map>
#include <memory>

namespace fetch {
namespace dmlf {

class LocalLearnerNetworker : public ILearnerNetworker
{
public:
  using PeerP = std::shared_ptr<LocalLearnerNetworker>;
  using Peers = std::vector<PeerP>;

  LocalLearnerNetworker();
  virtual ~LocalLearnerNetworker();
  virtual void        pushUpdate(std::shared_ptr<IUpdate> update);
  virtual std::size_t getUpdateCount() const;

  virtual std::size_t getPeerCount() const
  {
    return peers.size();
  }
  void addPeers(Peers new_peers);
  void clearPeers();

protected:
private:
  using Mutex            = fetch::Mutex;
  using Lock             = std::unique_lock<Mutex>;
  using Intermediate     = ILearnerNetworker::Intermediate;
  using IntermediateList = std::list<Intermediate>;

  IntermediateList updates;
  mutable Mutex    mutex;
  Peers            peers;

  virtual Intermediate getUpdateIntermediate();
  void                 rx(const Intermediate &data);

  LocalLearnerNetworker(const LocalLearnerNetworker &other) = delete;
  LocalLearnerNetworker &operator=(const LocalLearnerNetworker &other)  = delete;
  bool                   operator==(const LocalLearnerNetworker &other) = delete;
  bool                   operator<(const LocalLearnerNetworker &other)  = delete;
};

}  // namespace dmlf
}  // namespace fetch
