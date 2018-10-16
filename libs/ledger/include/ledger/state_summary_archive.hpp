#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/byte_array/const_byte_array.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "storage/object_store.hpp"

#include <chrono>
#include <cstdint>
#include <string>

namespace fetch {
namespace ledger {

/**
 * Keeps track of state hashes and there corresponding bookmark indexes.
 */
class StateSummaryArchive
{
public:
  using Hash     = StorageUnitInterface::hash_type;
  using Bookmark = StorageUnitInterface::bookmark_type;

  void New(std::string const &db_path, std::string const &index_path)
  {
    archive_.New(db_path, index_path);
  }

  /**
   * Lookup the bookmark associated with the given state hash
   *
   * @param state_hash The input state hash to lookup
   * @param bookmark The returned bookmark index to be used
   * @return true if successful, otherwise false
   */
  bool LookupBookmark(Hash const &state_hash, Bookmark &bookmark)
  {
    return archive_.Get(ResourceID{state_hash}, bookmark);
  }

  /**
   * Allocate a bookmark to a pending state hash. This is only persisted once the `ConfirmBookmark`
   * call has been made.
   *
   * By design it is assumed that the user of this object will attempt to lookup a bookmark for a
   * state hash before allocating a new once. Failure to do so will result in errors.
   *
   * @param state_hash The input state hash
   * @param bookmark The input book
   * @return true if successful, otherwise false
   */
  bool AllocateBookmark(Hash const &state_hash, Bookmark &bookmark)
  {
    Bookmark dummy{0};
    if (archive_.Get(ResourceID{state_hash}, dummy))
    {
      return false;
    }

    // check that there isn't already the same hash in the pending queue
    auto it = pending_.find(state_hash);
    if (it != pending_.end())
    {
      bookmark = it->second.bookmark;
      return true;
    }

    // allocate a bookmark index and store it in the pending queue
    bookmark = next_bookmark_++;
    pending_.emplace(state_hash, bookmark);

    return true;
  }

  /**
   * Confirm a pending bookmark
   *
   * Confirming a pending bookmark will ensure that the value is persisted. Also periodically this
   * function will be used to clean up the pending cache
   *
   * @param state_hash The pending bookmark state hash
   * @param bookmark The pending bookmark number
   * @return true if successful, otherwise false
   */
  bool ConfirmBookmark(Hash const &state_hash, Bookmark const &bookmark)
  {
    bool success = false;

    // lookup the state hash in the pending pool
    auto it = pending_.find(state_hash);
    if ((it != pending_.end()) && (it->second.bookmark == bookmark))
    {
      // update the archive
      archive_.Set(ResourceID{state_hash}, bookmark);

      // remove the pending entry
      pending_.erase(state_hash);

      success = true;
    };

    // perform cleanup on the cache if needed
    ++confirm_count_;
    if ((confirm_count_ & CLEANUP_PERIOD_MASK) == 0)
    {
      CleanUp();
    }

    return success;
  }

private:
  using Clock      = std::chrono::high_resolution_clock;
  using Timestamp  = Clock::time_point;
  using ResourceID = storage::ResourceID;
  using Archive    = storage::ObjectStore<Bookmark>;

  struct CacheElement
  {
    CacheElement() = default;
    CacheElement(Bookmark b)
      : bookmark{b}
    {}

    Bookmark  bookmark{0};               ///< The bookmark being stored
    Timestamp created_at{Clock::now()};  ///< The timestamp for the cache element
  };

  using PendingMap = std::unordered_map<Hash, CacheElement>;

  static constexpr uint32_t CLEANUP_PERIOD_MASK = 0x1F;

  void CleanUp()
  {
    // compute the current deadline for old pending tasks
    Timestamp const deadline = Clock::now() - std::chrono::hours{1};

    // loop through the pending map
    auto it = pending_.begin();
    while (it != pending_.end())
    {
      if (it->second.created_at < deadline)
      {
        // erase this entry which has timed out
        it = pending_.erase(it);
      }
      else
      {
        // move on to the next element
        ++it;
      }
    }
  }

  uint32_t   confirm_count_{0};  ///< Confirm call count, used for periodic work
  Bookmark   next_bookmark_{1};  ///< The next bookmark to allocated
  Archive    archive_;           ///< The persistent storage of state block indexes
  PendingMap pending_;           ///< In memory map of pending state hash vs. bookmark indexes
};

}  // namespace ledger
}  // namespace fetch
