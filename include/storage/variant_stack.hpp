#ifndef STORAGE_VARIANT_STACK_HPP
#define STORAGE_VARIANT_STACK_HPP
#include "byte_array/referenced_byte_array.hpp"

#include <cassert>
#include <fstream>

// TODO: Make variant stack as a circular buffer!

namespace fetch {
namespace storage {

class VariantStack {
 public:
  typedef byte_array::ReferencedByteArray byte_array_type;
  struct Separator {
    uint64_t type;
    uint64_t object_size;
    uint64_t next;
    uint64_t previous;

    bool Write(std::fstream &stream) const {
      if ((!stream) || (!stream.is_open())) return false;
      stream.write(reinterpret_cast<char const *>(&type), sizeof(type));
      stream.write(reinterpret_cast<char const *>(&object_size),
                   sizeof(object_size));
      stream.write(reinterpret_cast<char const *>(&next), sizeof(next));
      stream.write(reinterpret_cast<char const *>(&previous), sizeof(previous));
      return bool(stream);
    }

    bool Read(std::fstream &stream) {
      if ((!stream) || (!stream.is_open())) return false;
      stream.read(reinterpret_cast<char *>(&type), sizeof(type));
      stream.read(reinterpret_cast<char *>(&object_size), sizeof(object_size));
      stream.read(reinterpret_cast<char *>(&next), sizeof(next));
      stream.read(reinterpret_cast<char *>(&previous), sizeof(previous));

      return bool(stream);
    }

    constexpr std::size_t size() const {
      return sizeof(type) + sizeof(object_size) + sizeof(next) +
             sizeof(previous);
    }
  };

  struct Header {
    uint64_t start = Header::size();
    uint64_t end = Header::size();
    uint64_t last = uint64_t(-1);
    uint64_t object_count = 0;

    bool Write(std::fstream &stream) const {
      if ((!stream) || (!stream.is_open())) return false;
      stream.seekg(0, stream.beg);
      stream.write(reinterpret_cast<char const *>(&start), sizeof(start));
      stream.write(reinterpret_cast<char const *>(&end), sizeof(end));
      stream.write(reinterpret_cast<char const *>(&last), sizeof(last));
      stream.write(reinterpret_cast<char const *>(&object_count),
                   sizeof(object_count));
      return bool(stream);
    }

    bool Read(std::fstream &stream) {
      if ((!stream) || (!stream.is_open())) return false;
      stream.seekg(0, stream.beg);
      stream.read(reinterpret_cast<char *>(&start), sizeof(start));
      stream.read(reinterpret_cast<char *>(&end), sizeof(end));
      stream.read(reinterpret_cast<char *>(&last), sizeof(last));
      stream.read(reinterpret_cast<char *>(&object_count),
                  sizeof(object_count));
      return bool(stream);
    }

    constexpr std::size_t size() const {
      return sizeof(start) + sizeof(end) + sizeof(last);
    }
  };

  void Load(byte_array_type const &filename) {
    filename_ = filename;
    std::fstream fin(filename_,
                     std::ios::in | std::ios::out | std::ios::binary);
    header_.Read(fin);
  }

  void New(byte_array_type const &filename) {
    filename_ = filename;
    Clear();
  }

  enum { UNDEFINED_POSITION = uint64_t(-1) };

  template <typename T>
  void Push(T const &object, uint64_t const &type = 0) {
    std::fstream fin(filename_,
                     std::ios::in | std::ios::out | std::ios::binary);
    fin.seekg(header_.end, fin.beg);

    Separator separator = {type, sizeof(T), UNDEFINED_POSITION, header_.last};

    fin.write(reinterpret_cast<char const *>(&separator), sizeof(Separator));
    fin.write(reinterpret_cast<char const *>(&object), sizeof(T));

    fin.seekg(header_.last, fin.beg);
    fin.read(reinterpret_cast<char *>(&separator), sizeof(Separator));
    separator.next = header_.end;
    fin.seekg(header_.last, fin.beg);
    fin.write(reinterpret_cast<char const *>(&separator), sizeof(Separator));

    header_.last = header_.end;
    header_.end += sizeof(T) + sizeof(Separator);
    ++header_.object_count;
    header_.Write(fin);

    fin.close();
  }

  void Pop() {
    std::fstream fin(filename_,
                     std::ios::in | std::ios::out | std::ios::binary);
    fin.seekg(header_.last, fin.beg);
    Separator separator;

    fin.read(reinterpret_cast<char *>(&separator), sizeof(Separator));

    header_.end = header_.last;
    header_.last = separator.previous;
    --header_.object_count;
    header_.Write(fin);

    fin.close();
  }

  template <typename T>
  uint64_t Top(T &object) {
    std::fstream fin(filename_,
                     std::ios::in | std::ios::out | std::ios::binary);
    fin.seekg(header_.last, fin.beg);
    Separator separator;

    fin.read(reinterpret_cast<char *>(&separator), sizeof(Separator));
    if (separator.object_size != sizeof(T)) {
      std::cerr << "TODO: Throw error, size mismatch in variantstack"
                << std::endl;
      exit(-1);
    }

    fin.read(reinterpret_cast<char *>(&object), sizeof(T));
    fin.close();
    return separator.type;
  }

  uint64_t Type() {
    std::fstream fin(filename_,
                     std::ios::in | std::ios::out | std::ios::binary);
    fin.seekg(header_.last, fin.beg);
    Separator separator;

    fin.read(reinterpret_cast<char *>(&separator), sizeof(Separator));

    fin.close();
    return separator.type;
  }

  void Clear() {
    assert(filename_ != "");
    std::fstream fin(filename_, std::ios::out | std::ios::binary);

    header_ = Header();

    if (!header_.Write(fin)) {
      std::cerr << "Error: could not write header" << std::endl;
      exit(-1);
    }

    fin.close();
  }

  bool empty() const { return header_.start == header_.end; }
  std::size_t size() const { return header_.object_count; }

 private:
  byte_array_type filename_ = "";
  Header header_;
};
};
};

#endif
