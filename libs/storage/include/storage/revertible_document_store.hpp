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

#include "core/byte_array/const_byte_array.hpp"
#include "storage/document_store.hpp"

namespace fetch {
namespace storage {

class RevertibleDocumentStore
  : public DocumentStore<2048, FileBlockType<2048>, KeyValueIndex<>,
                         VersionedRandomAccessStack<FileBlockType<2048>>,
                         FileObject<VersionedRandomAccessStack<FileBlockType<2048>>>>
{
public:
  using hash_type  = byte_array::ConstByteArray;
  using super_type = DocumentStore<2048, FileBlockType<2048>, KeyValueIndex<>,
                                   VersionedRandomAccessStack<FileBlockType<2048>>,
                                   FileObject<VersionedRandomAccessStack<FileBlockType<2048>>>>;

  using bookmark_type = uint64_t;

  hash_type Hash()
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    return key_index_.Hash();
  }

  bookmark_type Commit(bookmark_type const &b)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);

    file_store_.Commit(b);
    return key_index_.Commit(b);
  }

  void Revert(bookmark_type const &b)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);

    std::cout << "First revert" << std::endl;
    file_store_.Revert(b);
    std::cout << "Second revert" << std::endl;

    key_index_.Revert(b);
  }

protected:
  DocumentFile GetDocumentFile(ResourceID const &rid, bool const &create = true)
  {
    return super_type::GetDocumentFile(rid, create);
  }
};

}  // namespace storage
}  // namespace fetch
