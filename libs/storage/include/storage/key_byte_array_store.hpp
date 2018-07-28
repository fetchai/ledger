
#pragma once

#include "storage/document_store.hpp"
namespace fetch {
namespace storage {

namespace details {

template <std::size_t BS = 2048>
struct ByteArrayMapConfigurator
{
  typedef KeyValuePair<>                               kvi_pair_type;
  typedef RandomAccessStack<kvi_pair_type, uint64_t>   kvi_stack_type;
  typedef KeyValueIndex<kvi_pair_type, kvi_stack_type> kvi_store_type;

  typedef FileBlockType<BS>                  file_block_type;
  typedef RandomAccessStack<file_block_type> document_stack_type;
  typedef FileObject<document_stack_type>    file_object_type;

  typedef DocumentStore<BS,
          file_block_type,
          kvi_store_type,
          document_stack_type,
          file_object_type> type;
};

}  // namespace details

template <std::size_t S>
using KeyByteArrayStore = typename details::ByteArrayMapConfigurator<S>::type;

}  // namespace storage
}  // namespace fetch
