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

#include "storage/document_store.hpp"
#include "storage/random_access_stack.hpp"

namespace fetch {
namespace storage {

namespace details {

template <std::size_t BLOCK_SIZE = 2048>
struct ByteArrayMapConfigurator
{
  using KVIPairType  = KeyValuePair<>;
  using KVIStackType = RandomAccessStack<KVIPairType, uint64_t>;

  using KVIStoreType = KeyValueIndex<KVIPairType, KVIStackType>;

  using SpecificFileBlockType = FileBlockType<BLOCK_SIZE>;
  using DocumentStackType     = RandomAccessStack<SpecificFileBlockType>;
  using FileObjectType        = FileObject<DocumentStackType>;

  using Type = DocumentStore<BLOCK_SIZE, SpecificFileBlockType, KVIStoreType, DocumentStackType,
                             FileObjectType>;
};

}  // namespace details

// this is simply a cleaner way of defining the template parameters to
// DocumentStore
template <std::size_t S>
using KeyByteArrayStore = typename details::ByteArrayMapConfigurator<S>::Type;

}  // namespace storage
}  // namespace fetch
