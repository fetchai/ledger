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

#include "storage/random_access_stack.hpp"
#include "storage/document_store.hpp"

namespace fetch {
namespace storage {

namespace details {

template <std::size_t BLOCK_SIZE = 2048>
struct ByteArrayMapConfigurator
{
  using kvi_pair_type  = KeyValuePair<>;
  using kvi_stack_type = RandomAccessStack<kvi_pair_type, uint64_t>;

  using kvi_store_type = KeyValueIndex<kvi_pair_type, kvi_stack_type>;

  using file_block_type     = FileBlockType<BLOCK_SIZE>;
  using document_stack_type = RandomAccessStack<file_block_type>;
  using file_object_type    = FileObject<document_stack_type>;

  using type = DocumentStore<BLOCK_SIZE, file_block_type, kvi_store_type, document_stack_type,
                             file_object_type>;
};

}  // namespace details

// this is simply a cleaner way of defining the template parameters to
// DocumentStore
template <std::size_t S>
using KeyByteArrayStore = typename details::ByteArrayMapConfigurator<S>::type;

}  // namespace storage
}  // namespace fetch
