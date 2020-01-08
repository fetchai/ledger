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

#include "chain/transaction_layout.hpp"
#include "core/digest.hpp"

#include <cstddef>
#include <cstdint>
#include <list>
#include <utility>

namespace fetch {
namespace ledger {

class TransactionLayoutQueue
{
public:
  using TransactionLayout = chain::TransactionLayout;
  using UnderlyingList    = std::list<TransactionLayout>;
  using Iterator          = UnderlyingList::iterator;
  using ConstIterator     = UnderlyingList::const_iterator;
  using TxLayoutSet       = std::unordered_set<TransactionLayout>;

  // Construction / Destruction
  TransactionLayoutQueue()                               = default;
  TransactionLayoutQueue(TransactionLayoutQueue const &) = delete;
  TransactionLayoutQueue(TransactionLayoutQueue &&)      = delete;
  ~TransactionLayoutQueue()                              = default;

  /// @name Iteration
  /// @{
  ConstIterator    cbegin() const;
  ConstIterator    begin() const;
  Iterator         begin();
  ConstIterator    cend() const;
  ConstIterator    end() const;
  Iterator         end();
  std::size_t      size() const;
  bool             empty() const;
  DigestSet const &digests() const;
  TxLayoutSet      TxLayouts() const;
  /// @}

  /// @name Basic Operations
  /// @{
  bool        Add(TransactionLayout const &item);
  bool        Remove(Digest const &digest);
  std::size_t Remove(DigestSet const &digests);
  void        Splice(TransactionLayoutQueue &other);
  void        Splice(TransactionLayoutQueue &other, Iterator start, Iterator end);
  Iterator    Erase(Iterator const &iterator);

  template <typename SortPredicate>
  void Sort(SortPredicate &&predicate);
  /// @}

  // Operators
  TransactionLayoutQueue &operator=(TransactionLayoutQueue const &) = delete;
  TransactionLayoutQueue &operator=(TransactionLayoutQueue &&) = delete;

private:
  DigestSet      digests_;  ///< Set of digests stored within the list
  UnderlyingList list_;     ///< The list of transaction layouts
};

template <typename SortPredicate>
void TransactionLayoutQueue::Sort(SortPredicate &&predicate)
{
  list_.sort(std::forward<SortPredicate>(predicate));
}

}  // namespace ledger
}  // namespace fetch
