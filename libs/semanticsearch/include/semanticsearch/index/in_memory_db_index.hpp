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

#include "semanticsearch/index/base_types.hpp"
#include "semanticsearch/index/database_index_interface.hpp"
#include "semanticsearch/index/semantic_subscription.hpp"
#include "semanticsearch/index/subscription_group.hpp"

#include <map>

namespace fetch {
namespace semanticsearch {

class InMemoryDBIndex : public DatabaseIndexInterface
{
public:
  void           AddRelation(SemanticSubscription const &obj) override;
  DBIndexListPtr Find(SemanticCoordinateType g, SemanticPosition position) const override;

private:
  std::map<SubscriptionGroup, DBIndexListPtr> compartments_;
  SemanticCoordinateType                      hc_width_param_start_ = 0;
  SemanticCoordinateType                      hc_width_param_end_   = 20;
};

}  // namespace semanticsearch
}  // namespace fetch
