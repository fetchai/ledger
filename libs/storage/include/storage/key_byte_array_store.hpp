#pragma once

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

// this is simply a cleaner way of defining the template parameters to DocumentStore
template <std::size_t S>
using KeyByteArrayStore = typename details::ByteArrayMapConfigurator<S>::type;

}  // namespace storage
}  // namespace fetch
