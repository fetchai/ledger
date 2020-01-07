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
#include "semanticsearch/index/database_index_interface.hpp"
#include "semanticsearch/index/subscription_group.hpp"
#include "semanticsearch/semantic_constants.hpp"

#include <map>

namespace fetch {
namespace semanticsearch {

/* Semantic search implements a reduction system that maps a datapoint
 * into a reduced dataspace. One such example is Word2Vec. The database index
 * maps these points in N dimensions to relevant data. The database index
 * implements search in an N-dimenensional hypercube. The search algorithm
 * uses hierachically related subscription groups as the data structure used
 * to perform the search. This makes search efficient if the radius for which
 * you are searching is known.
 *
 * The easiest way to understand the implementation is through a two-dimensional
 * example: If the 2D plane is bounded we can subdivide it into minor bounded planes
 * recursively. We have designed the system such that any axis for top-level plane is
 * always defined on the interval [0, 1] using unsigned integers to represent a
 * position along the axis. We refer to a plane or a subdivided plane as a
 * subscription group as these will hold information about the subscriptions
 * associated with this world segment.
 *
 * Given a position P, we can identify a number of subscription groups to which P
 * belong:
 *
 *                 ───────────────────────────────────────────────────────────
 *                 ╱                                                         ╱
 *                ╱                                                         ╱
 *               ╱                                                         ╱
 *              ╱                                                         ╱
 *             ╱                                                         ╱
 *            ╱                    SubscriptionGroup                    ╱
 *           ╱                                                         ╱
 *          ╱─ ─ ─ ─ ─ ─ ─ ─ ─ ─● P                                   ╱
 *         ╱                   ╱                                     ╱
 *        ╱                                                         ╱
 *       ╱                   ╱                                     ╱
 *      ╱                                                         ╱
 *     ───────────────────────────────────────────────────────────   depth = 0
 *
 *             ───────────────────────────────────────────────────────────
 *             ╱                            ╱                            ╱
 *            ╱                            ╱                            ╱
 *           ╱    SubscriptionGroup       ╱     SubscriptionGroup      ╱
 *          ╱                            ╱                            ╱
 *         ╱                            ╱                            ╱
 *        ╱────────────────────────────╳────────────────────────────╱
 *       ╱                            ╱                            ╱
 *      ╱─ ─ ─ ─ ─ ─ ─ ─ ─ ─● P      ╱                            ╱
 *     ╱                   ╱        ╱      SubscriptionGroup     ╱
 *    ╱                            ╱                            ╱
 *   ╱                   ╱        ╱                            ╱
 *  ╱                            ╱                            ╱
 * ───────────────────────────────────────────────────────────   depth = 1
 *
 *
 *             ───────────────────────────────────────────────────────────
 *             ╱             ╱              ╱             ╱              ╱
 *            ╱             ╱              ╱             ╱              ╱
 *           ╱─────────────╳──────────────╳─────────────╳──────────────╱
 *          ╱             ╱              ╱             ╱              ╱
 *         ╱             ╱              ╱             ╱              ╱
 *        ╱─────────────╳──────────────╳─────────────╳──────────────╱
 *       ╱             ╱              ╱             ╱              ╱
 *      ╱─ ─ ─ ─ ─ ─ ─╱─ ─ ─● P      ╱             ╱              ╱
 *     ╱─────────────╳─────╳────────╳─────────────╳──────────────╱
 *    ╱             ╱              ╱             ╱              ╱
 *   ╱             ╱     ╱        ╱             ╱              ╱
 *  ╱             ╱              ╱             ╱              ╱
 * ───────────────────────────────────────────────────────────   depth = 2
 *
 * The index keeps track of these subscription groups and the contents in
 * them.
 */

class InMemoryDBIndex : public DatabaseIndexInterface
{
public:
  using GroupToIndicesMap = std::map<SubscriptionGroup, DBIndexSetPtr>;

  InMemoryDBIndex(std::size_t rank);

  /// Methods to manage the database
  /// @{

  /* @brief Adds a relation between an index and a sematic position.
   * @param index Database index containing the record for the subscription.
   * @param position Position in semantic space.
   */
  void AddRelation(DBIndexType const &index, SemanticPosition const &position) override;

  /* @brief Finds a set of indices near a given position.
   * @param depth The depth level at which we search.
   * @param position The position for which we need to find a group.
   */
  DBIndexSetPtr Find(DepthParameterType depth, SemanticPosition position) const override;
  /// @}

  /// Properties
  /// @{
  std::size_t rank() const override;
  /// @}
private:
  GroupToIndicesMap  group_content_{};         ///< Mapping of group to set of indices.
  DepthParameterType param_depth_start_ = 0;   ///< Smallest depth searchable.
  DepthParameterType param_depth_end_{MAXIMUM_DEPTH};  ///< Largest depth searchable.
  std::size_t        rank_{0};                 ///< The rank of elements contained in the db.
};

}  // namespace semanticsearch
}  // namespace fetch
