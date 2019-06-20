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

#include "ledger/chain/digest.hpp"
#include "ledger/chain/transaction_layout.hpp"

#include <cstddef>
#include <cstdint>
#include <list>

namespace fetch {
namespace miner {

class TransactionLayoutQueue
{
public:
  using TransactionLayout = ledger::TransactionLayout;
  using Digest            = ledger::Digest;
  using DigestSet         = ledger::DigestSet;
  using UnderlyingList    = std::list<TransactionLayout>;
  using Iterator          = UnderlyingList::iterator;
  using ConstIterator     = UnderlyingList::const_iterator;

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

inline TransactionLayoutQueue::ConstIterator TransactionLayoutQueue::cbegin() const
{
  return list_.cbegin();
}

inline TransactionLayoutQueue::ConstIterator TransactionLayoutQueue::begin() const
{
  return list_.begin();
}

inline TransactionLayoutQueue::Iterator TransactionLayoutQueue::begin()
{
  return list_.begin();
}

inline TransactionLayoutQueue::ConstIterator TransactionLayoutQueue::cend() const
{
  return list_.cend();
}

inline TransactionLayoutQueue::ConstIterator TransactionLayoutQueue::end() const
{
  return list_.end();
}

inline TransactionLayoutQueue::Iterator TransactionLayoutQueue::end()
{
  return list_.end();
}

inline std::size_t TransactionLayoutQueue::size() const
{
  return digests_.size();
}

inline bool TransactionLayoutQueue::empty() const
{
  return digests_.empty();
}

inline TransactionLayoutQueue::DigestSet const &TransactionLayoutQueue::digests() const
{
  return digests_;
}

template <typename SortPredicate>
void TransactionLayoutQueue::Sort(SortPredicate &&predicate)
{
  list_.sort(predicate);
}

}  // namespace miner
}  // namespace fetch
