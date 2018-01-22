#ifndef STORAGE_INDEXED_DOCUMENT_STORE_HPP
#define STORAGE_INDEXED_DOCUMENT_STORE_HPP
#include "byte_array/referenced_byte_array.hpp"
#include"storage/file_object.hpp"
#include <cassert>
#include <fstream>
namespace fetch {
namespace storage {


template <typename B>
class IndexedDocumentStore {
 public:
  typedef B block_type;
  typedef byte_array::ReferencedByteArray byte_array_type;
  void Load(byte_array_type const &filename) {}
  void New(byte_array_type const &filename) {}

  //  FileObject Get(std::size_t const &i) {}

  //  FileObject New() {}

  void Delete(std::size_t const &j) {}

  void Clear() {}

 private:
  byte_array_type filename_ = "";
};
};
};

#endif
