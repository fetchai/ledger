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

Mutex LocalLearnerNetworker::index_mutex;
LocalLearnerNetworker::LocalLearnerNetworkerIndex LocalLearnerNetworker::index;

LocalLearnerNetworker::LocalLearnerNetworker()
{
  this -> ident = getCounter()++;

  Lock lock(index_mutex);
  index[ident] = this;
}

LocalLearnerNetworker::~LocalLearnerNetworker()
{
}

void LocalLearnerNetworker::pushUpdate( std::shared_ptr<IUpdate> update)
{
  std::cout << update << std::endl;

  std::vector<LocalLearnerNetworker*> targets;
  auto indexes = alg -> getNextOutputs();

  {
    Lock lock(index_mutex);
    for(auto ind : indexes)
    {
      auto mapping = index.find(ind);
      if (mapping != index.end())
      {
        targets.push_back(mapping->second);
      }
    }
  }

  for(auto t : targets)
  {
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

std::size_t LocalLearnerNetworker::getCount()
{
  return getCounter();
}

std::size_t &LocalLearnerNetworker::getCounter()
{
  static std::size_t counter = 0;
  return counter;
}

}
}
