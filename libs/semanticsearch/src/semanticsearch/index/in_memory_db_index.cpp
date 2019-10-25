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

#include "semanticsearch/index/in_memory_db_index.hpp"

#include <cassert>

namespace fetch {
namespace semanticsearch {

void InMemoryDBIndex::AddRelation(SemanticSubscription const &obj)
{
  for (SemanticCoordinateType s = hc_width_param_start_; s < hc_width_param_end_; ++s)
  {
    SubscriptionGroup idx{s, obj.position};
    auto              it = compartments_.find(idx);

    if (it == compartments_.end())
    {
      DBIndexListPtr c = std::make_shared<DBIndexList>();
      c->insert(obj.index);
      compartments_.insert({idx, c});
    }
    else
    {
      it->second->insert(obj.index);
    }
  }
}

DBIndexListPtr InMemoryDBIndex::Find(SemanticCoordinateType g, SemanticPosition position) const
{
  SubscriptionGroup idx{g, position};

  auto it = compartments_.find(idx);
  if (it == compartments_.end())
  {
    return nullptr;
  }

  return it->second;
}

}  // namespace semanticsearch
}  // namespace fetch
