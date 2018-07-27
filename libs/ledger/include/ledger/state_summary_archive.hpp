#pragma once

#include "core/byte_array/const_byte_array.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <chrono>

namespace fetch {
namespace ledger {

class StateSummaryArchive
{
public:
  using hash_type         = StorageUnitInterface::hash_type;
  using bookmark_type     = StorageUnitInterface::bookmark_type;
  using block_digest_type = byte_array::ConstByteArray;
  using clock_type        = std::chrono::high_resolution_clock;
  using timepoint_type    = clock_type::time_point;

  struct Element
  {
    Element(bookmark_type b) : bookmark{b} {}

    bookmark_type  bookmark;
    bool           confirmed{false};
    timepoint_type timestamp{clock_type::now()};
  };

  using archive_type = std::unordered_map<hash_type, Element, crypto::CallableFNV>;

  bool LookupBookmark(hash_type const &state_hash, bookmark_type &bookmark)
  {
    bool success = false;

    auto it = archive_.find(state_hash);
    if ((it != archive_.end()) && it->second.confirmed)
    {
      bookmark = it->second.bookmark;
      success  = true;
    }

    return success;
  }

  bool RecordBookmark(hash_type const &state_hash, bookmark_type &bookmark)
  {

    // check to see if there is already a confirmed bookmark already with this
    // state hash
    auto it = archive_.find(state_hash);
    if ((it != archive_.end()) && it->second.confirmed)
    {
      return false;
    }

    // we have determined that this is the first time we have seen this hash so
    // we need to make a new hash
    bookmark = next_bookmark_++;
    archive_.emplace(state_hash, bookmark);

    return true;
  }

  bool ConfirmBookmark(hash_type const &state_hash, bookmark_type const &bookmark)
  {
    bool success = false;

    auto it = archive_.find(state_hash);
    if (it != archive_.end())
    {
      if (it->second.bookmark == bookmark)
      {
        it->second.confirmed = true;
        success              = true;
      }
    }

    return success;
  }

private:
  bookmark_type next_bookmark_{1};
  archive_type  archive_;
};

}  // namespace ledger
}  // namespace fetch
