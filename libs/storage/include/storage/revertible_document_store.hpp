#pragma once
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

  typedef uint64_t bookmark_type;  // TODO: From keyvalue index

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
