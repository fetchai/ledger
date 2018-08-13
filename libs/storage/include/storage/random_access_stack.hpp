#pragma once
#include <cassert>
#include <fstream>
#include <functional>
#include <string>

#include "core/assert.hpp"

namespace fetch {
namespace platform {
enum
{
  LITTLE_ENDIAN_MAGIC = 1337
};
}
namespace storage {

template <typename T, typename D = uint64_t>
class RandomAccessStack
{
private:
  struct Header
  {
    uint16_t magic   = platform::LITTLE_ENDIAN_MAGIC;
    uint64_t objects = 0;
    D        extra;

    bool Write(std::fstream &stream) const
    {
      if ((!stream) || (!stream.is_open())) return false;
      stream.seekg(0, stream.beg);
      stream.write(reinterpret_cast<char const *>(&magic), sizeof(magic));
      stream.write(reinterpret_cast<char const *>(&objects), sizeof(objects));
      stream.write(reinterpret_cast<char const *>(&extra), sizeof(extra));
      return bool(stream);
    }

    bool Read(std::fstream &stream)
    {
      if ((!stream) || (!stream.is_open())) return false;
      stream.seekg(0, stream.beg);
      stream.read(reinterpret_cast<char *>(&magic), sizeof(magic));
      stream.read(reinterpret_cast<char *>(&objects), sizeof(objects));
      stream.read(reinterpret_cast<char *>(&extra), sizeof(extra));
      return bool(stream);
    }

    constexpr std::size_t size() const { return sizeof(magic) + sizeof(objects) + sizeof(D); }
  };

public:
  using header_extra_type  = D;
  using type               = T;
  using event_handler_type = std::function<void()>;

  void ClearEventHandlers()
  {
    on_file_loaded_  = nullptr;
    on_before_flush_ = nullptr;
  }

  void OnFileLoaded(event_handler_type const &f) { on_file_loaded_ = f; }

  void OnBeforeFlush(event_handler_type const &f) { on_before_flush_ = f; }

  void SignalFileLoaded()
  {
    if (on_file_loaded_) on_file_loaded_();
  }

  void SignalBeforeFlush()
  {
    if (on_before_flush_) on_before_flush_();
  }

  static constexpr bool DirectWrite() { return true; }

  ~RandomAccessStack()
  {
    if (file_handle_.is_open())
    {
      file_handle_.close();
    }
  }

  void Close(bool const &lazy = false)
  {
    if (!lazy) Flush();
    file_handle_.close();
  }

  void Load(std::string const &filename, bool const &create_if_not_exist = false)
  {
    filename_    = filename;
    file_handle_ = std::fstream(filename_, std::ios::in | std::ios::out | std::ios::binary);

    if (!file_handle_)
    {
      if (create_if_not_exist)
      {
        Clear();
        file_handle_ = std::fstream(filename_, std::ios::in | std::ios::out | std::ios::binary);
      }
      else
      {
        TODO_FAIL("Could not load file");
      }
    }

    file_handle_.seekg(0, file_handle_.end);
    int64_t length = file_handle_.tellg();
    file_handle_.seekg(0, file_handle_.beg);
    header_.Read(file_handle_);

    int64_t capacity = (length - int64_t(header_.size())) / int64_t(sizeof(type));
    assert(capacity >= 0);

    if (std::size_t(capacity) < header_.objects)
    {
      TODO_FAIL("Expected more stack objects.");
    }

    // TODO: Check magic

    SignalFileLoaded();
  }

  void New(std::string const &filename)
  {
    filename_ = filename;
    Clear();
    file_handle_ = std::fstream(filename_, std::ios::in | std::ios::out | std::ios::binary);

    SignalFileLoaded();
  }

  // TODO: Protected functions
  void Get(std::size_t const &i, type &object) const
  {
    assert(filename_ != "");
    assert(i < size());

    int64_t n = int64_t(i * sizeof(type) + header_.size());

    file_handle_.seekg(n);
    file_handle_.read(reinterpret_cast<char *>(&object), sizeof(type));
  }

  void Set(std::size_t const &i, type const &object)
  {
    assert(filename_ != "");
    assert(i < size());
    int64_t n = int64_t(i * sizeof(type) + header_.size());

    file_handle_.seekg(n, file_handle_.beg);
    file_handle_.write(reinterpret_cast<char const *>(&object), sizeof(type));
  }

  void SetExtraHeader(header_extra_type const &he)
  {
    assert(filename_ != "");

    header_.extra = he;
    StoreHeader();
  }

  header_extra_type const &header_extra() const { return header_.extra; }

  uint64_t Push(type const &object)
  {
    uint64_t ret = header_.objects;
    int64_t  n   = int64_t(ret * sizeof(type) + header_.size());

    file_handle_.seekg(n, file_handle_.beg);
    file_handle_.write(reinterpret_cast<char const *>(&object), sizeof(type));

    ++header_.objects;
    StoreHeader();
    return ret;
  }

  void Pop()
  {
    --header_.objects;
    StoreHeader();
  }

  type Top() const
  {
    assert(header_.objects > 0);

    int64_t n = int64_t((header_.objects - 1) * sizeof(type) + header_.size());

    file_handle_.seekg(n, file_handle_.beg);
    type object;
    file_handle_.read(reinterpret_cast<char *>(&object), sizeof(type));

    return object;
  }

  void Swap(std::size_t const &i, std::size_t const &j)
  {
    if (i == j) return;
    type a, b;
    assert(filename_ != "");

    int64_t n1 = int64_t(i * sizeof(type) + header_.size());
    int64_t n2 = int64_t(j * sizeof(type) + header_.size());

    file_handle_.seekg(n1);
    file_handle_.read(reinterpret_cast<char *>(&a), sizeof(type));
    file_handle_.seekg(n2);
    file_handle_.read(reinterpret_cast<char *>(&b), sizeof(type));

    file_handle_.seekg(n1);
    file_handle_.write(reinterpret_cast<char const *>(&b), sizeof(type));
    file_handle_.seekg(n2);
    file_handle_.write(reinterpret_cast<char const *>(&a), sizeof(type));
  }

  std::size_t size() const { return header_.objects; }

  std::size_t empty() const { return header_.objects == 0; }

  void Clear()
  {
    assert(filename_ != "");
    std::fstream fin(filename_, std::ios::out | std::ios::binary);
    header_ = Header();

    if (!header_.Write(fin))
    {
      TODO_FAIL("Error could not write header - todo throw error");
    }

    fin.close();
  }

  void Flush(bool const &lazy = false)
  {
    if (!lazy) SignalBeforeFlush();
    StoreHeader();
    file_handle_.flush();
  }

  bool is_open() const { return bool(file_handle_) && (file_handle_.is_open()); }

  void StoreHeader()
  {
    assert(filename_ != "");

    if (!header_.Write(file_handle_))
    {
      TODO_FAIL("Error could not write header - todo throw error");
    }
  }

  uint64_t LazyPush(type const &object)
  {
    uint64_t ret = header_.objects;
    int64_t  n   = int64_t(ret * sizeof(type) + header_.size());

    file_handle_.seekg(n, file_handle_.beg);
    file_handle_.write(reinterpret_cast<char const *>(&object), sizeof(type));
    ++header_.objects;

    return ret;
  }

private:
  event_handler_type   on_file_loaded_;
  event_handler_type   on_before_flush_;
  mutable std::fstream file_handle_;
  std::string          filename_ = "";
  Header               header_;
};
}  // namespace storage
}  // namespace fetch
