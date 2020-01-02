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

#include "semanticsearch/index/base_types.hpp"

namespace fetch {
namespace semanticsearch {
/*
 *      Semantic space (Two dimensional example)
 *
 *                    ───────────────────────────────────────────────────────────
 *                    ╱                                                         ╱
 *                   ╱                                                         ╱
 *                  ╱                                                         ╱
 *                 ╱                                                         ╱
 *                ╱                   SubscriptionGroup                     ╱
 *               ╱                                                         ╱
 *              ╱                    position = (0,0)                     ╱
 *             ╱                                                         ╱
 *            ╱                                                         ╱
 *           ╱                                                         ╱
 *          ╱                                                         ╱
 *         ╱                                                         ╱
 *        ───────────────────────────────────────────────────────────   depth = 0
 *
 *           ──── ───────────────────────────────────────────────────────────
 *     w      ╱   ╱                            ╱                            ╱
 *     i     ╱   ╱      SubscriptionGroup     ╱     SubscriptionGroup      ╱
 *     d    ╱   ╱                            ╱                            ╱
 *     t   ╱   ╱      position = (0,0)      ╱     position = (1,0)       ╱
 *     h  ╱   ╱                            ╱                            ╱
 *      ──── ╱────────────────────────────╳────────────────────────────╱
 *          ╱                            ╱                            ╱
 *         ╱       SubscriptionGroup    ╱      SubscriptionGroup     ╱
 *        ╱                            ╱                            ╱
 *       ╱       position = (0,1)     ╱      position = (1,1)      ╱
 *      ╱                            ╱                            ╱
 *     ╱                            ╱                            ╱
 *    ───────────────────────────────────────────────────────────   depth = 1
 *                                 │                            │
 *                                 │──────────  width  ─────────│
 *                                 │                            │
 */

struct SubscriptionGroup
{
  SubscriptionGroup() = default;
  SubscriptionGroup(SemanticCoordinateType d, SemanticPosition position);

  /*
   * @brief computes the size of group number g
   *
   * The larger the group number, the smaller the size.
   */
  static constexpr SemanticCoordinateType CalculateWidthFromDepth(SemanticCoordinateType depth)
  {
    return SemanticCoordinateType(-1) >> depth;
  }

  SemanticPosition       indices;
  SemanticCoordinateType depth;  ///< Parameter that determines the depth of the subscription

  bool operator<(SubscriptionGroup const &other) const;
  bool operator==(SubscriptionGroup const &other) const;
};

}  // namespace semanticsearch
}  // namespace fetch
