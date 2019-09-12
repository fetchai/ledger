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

#include "dmlf/local_learner_networker.hpp"

#include <iostream>

#include "dmlf/iupdate.hpp"

namespace fetch {
namespace dmlf {

LocalLearnerNetworker::LocalLearnerNetworker()
{
}

LocalLearnerNetworker::~LocalLearnerNetworker()
{
}

void LocalLearnerNetworker::addPeers(std::vector<std::shared_ptr<LocalLearnerNetworker>> new_peers)
{
  for(auto peer : new_peers)
  {
    peers.push_back(peer);
  }
}

void LocalLearnerNetworker::clearPeers()
{
  peers.clear();
}

void LocalLearnerNetworker::pushUpdate( std::shared_ptr<IUpdate> update)
{
  std::vector<std::shared_ptr<LocalLearnerNetworker>> targets;
  auto indexes = alg -> getNextOutputs();

  for(auto ind : indexes)
  {
    auto t = peers[ind];
    Lock lock(t -> mutex);
    t -> updates.push_back(update);
  }
}

std::size_t LocalLearnerNetworker::getUpdateCount() const
{
  Lock lock(mutex);
  return updates.size();
}

std::shared_ptr<IUpdate> LocalLearnerNetworker::getUpdate()
{
  Lock lock(mutex);

  if (!updates.empty())
  {
    auto x = updates.front();
    updates.pop_front();
    return x;
  }

  return std::shared_ptr<IUpdate>();
}

}
}
