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

#include "storage/new_revertible_document_store.hpp"

#include <cstddef>
#include <string>
#include <utility>

using Hash           = fetch::storage::NewRevertibleDocumentStore::Hash;
using ByteArray      = fetch::storage::NewRevertibleDocumentStore::ByteArray;
using UnderlyingType = fetch::storage::NewRevertibleDocumentStore::UnderlyingType;

static constexpr char const *LOGGING_NAME = "NewRevertibleDocStore";

namespace fetch {
namespace storage {
namespace {
bool IsAllZeros(Hash const &hash)
{
  bool all_zeros{true};

  for (std::size_t i = 0, end = hash.size(); i < end; ++i)
  {
    if (hash[i] > 0)
    {
      all_zeros = false;
      break;
    }
  }

  return all_zeros;
}
}  // namespace

bool NewRevertibleDocumentStore::Load(std::string const &state, std::string const &state_history,
                                      std::string const &index, std::string const &index_history,
                                      bool create = true)
{
  // cache the filenames
  state_path_         = state;
  state_history_path_ = state_history;
  index_path_         = index;
  index_history_path_ = index_history;

  // trigger the load
  storage_.Load(state, state_history, index, index_history, create);
  return true;
}

bool NewRevertibleDocumentStore::New(std::string const &state, std::string const &state_history,
                                     std::string const &index, std::string const &index_history,
                                     bool /*create*/ = true)
{
  // cache the filenames
  state_path_         = state;
  state_history_path_ = state_history;
  index_path_         = index;
  index_history_path_ = index_history;

  // trigger creation
  storage_.New(state, state_history, index, index_history);

  return true;
}

UnderlyingType NewRevertibleDocumentStore::Get(ResourceID const &rid)
{
  return storage_.Get(rid);
}

UnderlyingType NewRevertibleDocumentStore::GetOrCreate(ResourceID const &rid)
{
  return storage_.GetOrCreate(rid);
}

void NewRevertibleDocumentStore::Set(ResourceID const &rid, ByteArray const &value)
{
  return storage_.Set(rid, value);
}

void NewRevertibleDocumentStore::Erase(ResourceID const &rid)
{
  return storage_.Erase(rid);
}

// State-based operations
Hash NewRevertibleDocumentStore::Commit()
{
  Hash ret{std::move(storage_.Commit())};
  storage_.Flush(false);
  return ret;
}

bool NewRevertibleDocumentStore::RevertToHash(Hash const &state)
{
  bool success{false};

  if (IsAllZeros(state))
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Reverting database back to initial state");

    // we are requesting to revert to a blank slate. The simplest way to handle this is to clear
    // out the database
    storage_.New(state_path_, state_history_path_, index_path_, index_history_path_);

    success = true;
  }
  else
  {
    success = storage_.RevertToHash(state);
  }

  return success;
}

bool NewRevertibleDocumentStore::HashExists(Hash const &hash)
{
  return storage_.HashExists(hash);
}

Hash NewRevertibleDocumentStore::CurrentHash()
{
  return storage_.CurrentHash();
}

std::size_t NewRevertibleDocumentStore::size() const
{
  return storage_.size();
}

}  // namespace storage
}  // namespace fetch
