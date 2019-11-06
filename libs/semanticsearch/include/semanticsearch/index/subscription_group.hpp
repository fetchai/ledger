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

namespace fetch {
namespace semanticsearch {

struct SubscriptionGroup
{
  SubscriptionGroup() = default;
  SubscriptionGroup(SemanticCoordinateType g, SemanticPosition position);

  /*
   * @brief computes the size of group number g
   *
   * The larger the group number, the smaller the size.
   */
  static constexpr SemanticCoordinateType SubscriptionGroupSize(SemanticCoordinateType g)
  {
    return SemanticCoordinateType(-1) >> g;
  }

  SemanticPosition       indices;
  SemanticCoordinateType width_parameter;  ///< Parameter that determines the witdth

  bool operator<(SubscriptionGroup const &other) const;
  bool operator==(SubscriptionGroup const &other) const;
};

}  // namespace semanticsearch
}  // namespace fetch
