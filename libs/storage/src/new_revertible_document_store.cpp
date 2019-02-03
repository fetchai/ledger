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

/* static constexpr char const *LOGGING_NAME = "RevertableStore"; */

#include "storage/new_revertible_document_store.hpp"

namespace fetch {
namespace storage {

using Hash           = NewRevertibleDocumentStore::Hash;
using ByteArray      = NewRevertibleDocumentStore::ByteArray;
using UnderlyingType = NewRevertibleDocumentStore::UnderlyingType;

NewRevertibleDocumentStore::NewRevertibleDocumentStore()
{}

bool NewRevertibleDocumentStore::Load(std::string const &state, std::string const &state_history,
                                      std::string const &index, std::string const &index_history,
                                      bool create = true)
{
  storage_.Load(state, state_history, index, index_history, create);
  return true;
}

bool NewRevertibleDocumentStore::New(std::string const &state, std::string const &state_history,
                                     std::string const &index, std::string const &index_history,
                                     bool /*create*/ = true)
{
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

// State-based operations
Hash NewRevertibleDocumentStore::Commit()
{
  return storage_.Commit();
}

bool NewRevertibleDocumentStore::RevertToHash(Hash const &state)
{
  return storage_.RevertToHash(state);
}

bool NewRevertibleDocumentStore::HashExists(Hash const &hash)
{
  return storage_.HashExists(hash);
}

Hash NewRevertibleDocumentStore::CurrentHash()
{
  return storage_.CurrentHash();
}

}  // namespace storage
}  // namespace fetch
