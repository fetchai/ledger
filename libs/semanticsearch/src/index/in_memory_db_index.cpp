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

#include "semanticsearch/index/in_memory_db_index.hpp"

#include <cassert>

namespace fetch {
namespace semanticsearch {
InMemoryDBIndex::InMemoryDBIndex(std::size_t rank)
  : rank_{rank}
{}

void InMemoryDBIndex::AddRelation(SemanticSubscription const &obj)
{
  // We require that only relations with same rank as the predefined
  // rank can be used. This is to prevent trivial mistakes in the code.
  if (obj.position.size() != rank_)
  {
    throw std::runtime_error("Rank of position differs from index.");
  }

  // Next we create subscriptions at a number of desired depths.
  for (SemanticCoordinateType s = param_depth_start_; s < param_depth_end_; ++s)
  {
    SubscriptionGroup idx{s, obj.position};
    auto              it = group_content_.find(idx);

    if (it == group_content_.end())
    {
      DBIndexSetPtr c = std::make_shared<DBIndexSet>();
      c->insert(obj.index);
      group_content_.insert({idx, c});
    }
    else
    {
      it->second->insert(obj.index);
    }
  }
}

DBIndexSetPtr InMemoryDBIndex::Find(SemanticCoordinateType depth, SemanticPosition position) const
{
  // Again, only operations with same rank as index is allowed.
  if (position.size() != rank_)
  {
    throw std::runtime_error("Rank of position differs from index.");
  }

  SubscriptionGroup idx{depth, position};

  auto it = group_content_.find(idx);
  if (it == group_content_.end())
  {
    return nullptr;
  }

  return it->second;
}

std::size_t InMemoryDBIndex::rank() const
{
  return rank_;
}

}  // namespace semanticsearch
}  // namespace fetch
