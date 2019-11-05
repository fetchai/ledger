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

#include "semanticsearch/index/subscription_group.hpp"

#include <cassert>

namespace fetch {
namespace semanticsearch {

SubscriptionGroup::SubscriptionGroup(SemanticCoordinateType g, SemanticPosition position)
  : width_parameter(g)
{
  auto cs = SubscriptionGroupSize(width_parameter);
  for (auto const &p : position)
  {
    indices.push_back(p / cs);
  }
}

bool SubscriptionGroup::operator<(SubscriptionGroup const &other) const
{
  if (indices.size() != other.indices.size())
  {
    throw std::runtime_error("DBIndexListPtrs below to different vector spaces.");
  }

  if (width_parameter != other.width_parameter)
  {
    return width_parameter < other.width_parameter;
  }

  std::size_t i = 0;
  while ((i != indices.size()) && (indices[i] == other.indices[i]))
  {
    ++i;
  }

  if (i == indices.size())
  {
    return false;
  }

  return indices[i] < (other.indices[i]);
}

bool SubscriptionGroup::operator==(SubscriptionGroup const &other) const
{
  if (indices.size() != other.indices.size())
  {
    throw std::runtime_error("DBIndexListPtrs below to different vector spaces.");
  }

  std::size_t i = 0;
  while ((i != indices.size()) && (indices[i] == other.indices[i]))
  {
    ++i;
  }

  return (width_parameter == other.width_parameter) && (i == indices.size());
}

}  // namespace semanticsearch
}  // namespace fetch
