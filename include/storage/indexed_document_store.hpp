#ifndef STORAGE_INDEXED_DOCUMENT_STORE_HPP
#define STORAGE_INDEXED_DOCUMENT_STORE_HPP
#include "byte_array/referenced_byte_array.hpp"

#include <cassert>
#include <fstream>
namespace fetch {
namespace storage {

template <typename B>
class FileObject {
 public:
  typedef B block_type;
  typedef byte_array::ReferencedByteArray byte_array_type;

  void Seek(std::size_t const &n) {}
  std::size_t Tell() const {}
  void Write(uint8_t const *, std::size_t const &n) {}
  void Read(uint8_t *, std::size_t const &n) const {}
  void Close() {}
  HashType hash() {}

 private:
  std::size_t start_ = 0;
  std::size_t position_ = 0;
  byte_array_type filename_ = ""
};

template <typename B>
class IndexedDocumentStore {
 public:
  typedef B block_type;
  typedef byte_array::ReferencedByteArray byte_array_type;
  void Load(byte_array_type const &filename) {}
  void New(byte_array_type const &filename) {}

  FileObject Get(std::size_t const &i) {}

  FileObject New() {}

  void Delete(std::size_t const &j) {}

  void Clear() {}

 private:
  byte_array_type filename_ = ""
};
};
};
