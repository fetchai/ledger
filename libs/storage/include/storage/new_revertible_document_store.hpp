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

#include "storage/document_store.hpp"
#include "storage/new_versioned_random_access_stack.hpp"

#include <string>

namespace fetch {
namespace storage {

class NewRevertibleDocumentStore
{
public:
  using Hash           = byte_array::ConstByteArray;
  using ByteArray      = byte_array::ConstByteArray;
  using UnderlyingType = storage::Document;
  using Keys           = std::vector<ResourceID>;

  bool New(std::string const &state, std::string const &state_history, std::string const &index,
           std::string const &index_history, bool create_if_not_exist);
  bool Load(std::string const &state, std::string const &state_history, std::string const &index,
            std::string const &index_history, bool create_if_not_exist);

  UnderlyingType Get(ResourceID const &rid);
  UnderlyingType GetOrCreate(ResourceID const &rid);
  void           Set(ResourceID const &rid, ByteArray const &value);
  void           Erase(ResourceID const &rid);

  Hash Commit();
  bool RevertToHash(Hash const &hash);
  Hash CurrentHash();
  bool HashExists(Hash const &hash);
  Keys KeyDump();

  std::size_t size() const;

private:
  using Storage = storage::DocumentStore<
      2048,                 // block size
      FileBlockType<2048>,  // file block type
      KeyValueIndex<KeyValuePair<>, NewVersionedRandomAccessStack<KeyValuePair<>>>,  // Key value
                                                                                     // index
      NewVersionedRandomAccessStack<FileBlockType<2048>>>;                           // File store

  std::string state_path_;
  std::string state_history_path_;
  std::string index_path_;
  std::string index_history_path_;
  Storage     storage_;
};

}  // namespace storage
}  // namespace fetch
