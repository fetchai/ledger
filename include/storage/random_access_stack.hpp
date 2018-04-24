#ifndef STORAGE_RANDOM_ACCESS_STACK_HPP
#define STORAGE_RANDOM_ACCESS_STACK_HPP
#include"assert.hpp"
#include <fstream>
#include <string>

namespace fetch {
namespace platform {
enum { LITTLE_ENDIAN_MAGIC = 1337 };
}
namespace storage {

template <typename T, typename D = uint64_t>
class RandomAccessStack {
 private:
  struct Header {
    uint64_t magic = platform::LITTLE_ENDIAN_MAGIC;
    uint64_t objects = 0;
    D extra;

    bool Write(std::fstream &stream) const {
      if ((!stream) || (!stream.is_open())) return false;
      stream.seekg(0, stream.beg);
      stream.write(reinterpret_cast<char const *>(&magic), sizeof(magic));
      stream.write(reinterpret_cast<char const *>(&objects), sizeof(objects));
      stream.write(reinterpret_cast<char const *>(&extra), sizeof(extra));
      return bool(stream);
    }

    bool Read(std::fstream &stream) {
      if ((!stream) || (!stream.is_open())) return false;
      stream.seekg(0, stream.beg);
      stream.read(reinterpret_cast<char *>(&magic), sizeof(magic));
      stream.read(reinterpret_cast<char *>(&objects), sizeof(objects));
      stream.read(reinterpret_cast<char *>(&extra), sizeof(extra));
      return bool(stream);
    }

    constexpr std::size_t size() const {
      return sizeof(magic) + sizeof(objects) + sizeof(D);
    }
  };

 public:
  typedef D header_extra_type;
  typedef T type;

  void Load(std::string const &filename) {
    filename_ = filename;
    std::fstream fin(filename_,
                     std::ios::in | std::ios::out | std::ios::binary);
    if (!fin) {
      Clear();
      fin = std::fstream(filename_,
                         std::ios::in | std::ios::out | std::ios::binary);
    }

    fin.seekg(0, fin.end);
    int64_t length = fin.tellg();
    fin.seekg(0, fin.beg);
    header_.Read(fin);
    std::size_t capacity = (length - header_.size()) / sizeof(type);

    if (capacity < header_.objects) {
      TODO_FAIL( "Expected more stack objects." );
    }

    // TODO: Check magic

    fin.close();
  }

  void New(std::string const &filename) {
    filename_ = filename;
    Clear();
  }

  void Get(std::size_t const &i, type &object) const {
    detailed_assert(filename_ != "");
    int64_t n = int64_t(i * sizeof(type) + header_.size());
    std::fstream fin(filename_,
                     std::ios::in | std::ios::out | std::ios::binary);
    fin.seekg(n);
    fin.read(reinterpret_cast<char *>(&object), sizeof(type));
    fin.close();
  }

  void Set(std::size_t const &i, type const &object) const {
    detailed_assert(filename_ != "");
    int64_t n = int64_t(i * sizeof(type) + header_.size());
    std::fstream fin(filename_,
                     std::ios::in | std::ios::out | std::ios::binary);
    fin.seekg(n, fin.beg);
    fin.write(reinterpret_cast<char const *>(&object), sizeof(type));
    fin.close();
  }

  void SetExtraHeader(header_extra_type const &he) {
    detailed_assert(filename_ != "");
    std::fstream fin(filename_,
                     std::ios::in | std::ios::out | std::ios::binary);
    header_.extra = he;
    header_.Write(fin);
    fin.close();
  }

  header_extra_type header_extra() const { return header_.extra; }

  void Push(type const &object) {
    int64_t n = int64_t(header_.objects * sizeof(type) + header_.size());
    std::fstream fin(filename_,
                     std::ios::in | std::ios::out | std::ios::binary);
    fin.seekg(n, fin.beg);
    fin.write(reinterpret_cast<char const *>(&object), sizeof(type));

    ++header_.objects;
    header_.Write(fin);

    fin.close();
  }

  void Pop() {
    std::fstream fin(filename_,
                     std::ios::in | std::ios::out | std::ios::binary);

    --header_.objects;
    header_.Write(fin);

    fin.close();
  }

  type Top() const {
    detailed_assert(header_.objects > 0);

    int64_t n = int64_t( (header_.objects - 1) * sizeof(type) + header_.size() );
    std::fstream fin(filename_,
                     std::ios::in | std::ios::out | std::ios::binary);
    fin.seekg(n, fin.beg);

    type object;

    fin.read(reinterpret_cast<char *>(&object), sizeof(type));
    fin.close();

    return object;
  }

  void Swap(std::size_t const &i, std::size_t const &j) {
    if (i == j) return;
    type a, b;
    detailed_assert(filename_ != "");

    int64_t n1 = int64_t(i * sizeof(type) + header_.size());
    int64_t n2 = int64_t(j * sizeof(type) + header_.size());
    std::fstream fin(filename_,
                     std::ios::in | std::ios::out | std::ios::binary);

    fin.seekg(n1);
    fin.read(reinterpret_cast<char *>(&a), sizeof(type));
    fin.seekg(n2);
    fin.read(reinterpret_cast<char *>(&b), sizeof(type));

    fin.seekg(n1);
    fin.write(reinterpret_cast<char const *>(&b), sizeof(type));
    fin.seekg(n2);
    fin.write(reinterpret_cast<char const *>(&a), sizeof(type));

    fin.close();
  }

  std::size_t size() const { return header_.objects; }

  std::size_t empty() const { return header_.objects == 0; }

  void Clear() {
    detailed_assert(filename_ != "");
    std::fstream fin(filename_, std::ios::out | std::ios::binary);

    header_ = Header();

    if (!header_.Write(fin)) {
      TODO_FAIL( "Error could not write header - todo throw error" );
    }

    fin.close();
  }

 private:
  std::string filename_ = "";
  Header header_;
};
}
}

#endif
