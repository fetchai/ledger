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

#include "miner/transaction_layout_queue.hpp"

#include <algorithm>

namespace fetch {
namespace miner {


bool TransactionLayoutQueue::RemapAndAdd(UnderlyingList &list, TransactionLayout const &tx, uint32_t num_lanes)
{
  bool success{false};

  // remap the transaction layout
  BitVector mask{num_lanes};
  if (tx.mask().RemapTo(mask))
  {
    // construct the new transaction layout
    list.emplace_back(tx, mask);

    success = true;
  }

  return success;
}

/**
 * Adds a transcaction layout to the queue
 *
 * @param item The transaction layout to be added to the queue
 * @return true if successful, otherwise false
 */
bool TransactionLayoutQueue::Add(TransactionLayout const &item)
{
  bool success{false};

  auto const &digest = item.digest();

  // ensure that this isn't already a duplicate transaction layout
  if (digests_.find(digest) == digests_.end())
  {
    if (RemapAndAdd(list_, item, 1u << log2_num_lanes_))
    {
      // update the digest set
      digests_.insert(digest);

      success = true;
    }
  }

  return success;
}

/**
 * Remove a transaction specified by a digest
 *
 * @param digest The digest of the transaction layout to be removed
 * @return true if successful, otherwise false
 */
bool TransactionLayoutQueue::Remove(Digest const &digest)
{
  bool success{false};

  // attempt to find the target digest
  auto const it = std::find_if(
    list_.begin(),
    list_.end(),
    [&digest](TransactionLayout const &layout) {
      return layout.digest() == digest;
    }
  );

  // if we found the target element then remove the list entry and the digest from the set
  if (list_.end() != it)
  {
    list_.erase(it);
    digests_.erase(digest);

    success = true;
  }

  return success;
}

/**
 * Remove a set of transactions from the queue specified by the input set
 *
 * @param digests The set of the transaction digests to be removed
 * @return The number of transaction layouts removed from the queue
 */
std::size_t TransactionLayoutQueue::Remove(DigestSet const &digests)
{
  std::size_t count{0};

  if (!digests.empty())
  {
    list_.remove_if([this, &digests, &count](TransactionLayout const &layout) {
      auto const &digest = layout.digest();

      // determine if we should remove this element
      bool const remove = digests.find(digest) != digests.end();

      // also update the digests set too
      if (remove)
      {
        digests_.erase(digest);
        ++count;
      }

      return remove;
    });
  }

  return count;
}

/**
 * Splice the contents of the specified queue onto the end of the current queue
 *
 * After the operation the contents of the input queue will be zero
 *
 * @param other The queue to be spliced on to the end of the existing one
 */
void TransactionLayoutQueue::Splice(TransactionLayoutQueue &other)
{
  // extract the queue from other queue
  UnderlyingList input = std::move(other.list_);
  other.digests_.clear();

  // loop through the queue and filter out the duplicate entries from the main queue
  for (auto it = input.begin(); it != input.end();)
  {
    bool const new_entry = digests_.find(it->digest()) == digests_.end();

    if (new_entry)
    {
      // update this queues digest set (in anticipation for new transaction objects)
      digests_.insert(it->digest());

      // advance on to the next entry
      ++it;
    }
    else
    {
      // remove the entry from the list
      it = other.list_.erase(it);
    }
  }

  // splice the contents
  list_.splice(list_.end(), std::move(input));
}

void TransactionLayoutQueue::Splice(TransactionLayoutQueue &other, Iterator start, Iterator end)
{
  // Step 1. Determine the first in the subset which is a non-duplicate
  Iterator current{start};
  while (current != end)
  {
    auto const &digest = current->digest();

    if (digests_.find(digest) == digests_.end())
    {
      break;
    }

    // duplicate - remove the element from the "other" list
    other.digests_.erase(digest);
    current = other.list_.erase(current);
  }

  // if all the elements are duplicate exit early
  if (current == end)
  {
    return;
  }

  // Step 2. We now have at least one element in the range which must be copied
  Iterator const begin{current};

  // loop through the queue and filter out the duplicate entries from the main queue
  while (current != end)
  {
    auto const &digest = current->digest();

    // remove the digest item from the "other" queue
    other.digests_.erase(digest);

    // determine if this is a new entry or not
    bool const new_entry = digests_.find(digest) == digests_.end();

    if (new_entry)
    {
      // update this queues digest set (in anticipation for new transaction objects)
      digests_.insert(digest);

      // advance on to the next entry
      ++current;
    }
    else
    {
      // remove the entry from the list
      current = other.list_.erase(current);
    }
  }

  // splice the contents of the new range into the
  list_.splice(list_.end(), other.list_, begin, end);
}

TransactionLayoutQueue::Iterator TransactionLayoutQueue::Erase(Iterator const &iterator)
{
  // remove the associated digest from the set
  digests_.erase(iterator->digest());

  // remove the list element
  return list_.erase(iterator);
}

} // namespace miner
} // namespace fetch