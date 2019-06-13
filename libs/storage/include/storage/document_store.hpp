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

#include "core/byte_array/byte_array.hpp"
#include "storage/file_object.hpp"
#include "storage/key_value_index.hpp"
#include "storage/resource_mapper.hpp"

#include <cassert>
#include <fstream>
#include <memory>

#include "core/mutex.hpp"
#include "network/service/protocol.hpp"
#include "storage/document.hpp"

namespace fetch {
namespace storage {

/**
 * DocumentStore maps keys to serialized data (documents) which is stored on
 * your filesystem
 *
 * To do this it maintains two files, a file that stores a mapping of the keys
 * to locations
 * in the document store
 *
 */
template <std::size_t BLOCK_SIZE = 2048, typename A = FileBlockType<BLOCK_SIZE>,
          typename B = KeyValueIndex<>, typename C = VersionedRandomAccessStack<A>,
          typename D = FileObject<C>>
class DocumentStore
{
public:
  using self_type       = DocumentStore<BLOCK_SIZE, A, B, C, D>;
  using byte_array_type = byte_array::ByteArray;

  using file_block_type      = A;
  using key_value_index_type = B;
  using file_object_type     = D;

  using hash_type = byte_array::ConstByteArray;

  using index_type = typename key_value_index_type::index_type;

  static constexpr char const *LOGGING_NAME = "DocumentStore";

  DocumentStore()                         = default;
  DocumentStore(DocumentStore const &rhs) = delete;
  DocumentStore(DocumentStore &&rhs)      = delete;
  DocumentStore &operator=(DocumentStore const &rhs) = delete;
  DocumentStore &operator=(DocumentStore &&rhs) = delete;

  void Load(std::string const &doc_file, std::string const &doc_diff, std::string const &index_file,
            std::string const &index_diff, bool const &create = true)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    file_object_.Load(doc_file, doc_diff, create);
    key_index_.Load(index_file, index_diff, create);
  }

  void New(std::string const &doc_file, std::string const &doc_diff, std::string const &index_file,
           std::string const &index_diff)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    file_object_.New(doc_file, doc_diff);
    key_index_.New(index_file, index_diff);
  }

  void Load(std::string const &doc_file, std::string const &index_file, bool const &create = true)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    file_object_.Load(doc_file, create);
    key_index_.Load(index_file, create);
  }

  void New(std::string const &doc_file, std::string const &index_file)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    file_object_.New(doc_file);
    key_index_.New(index_file);
  }

  Document GetOrCreate(ResourceID const &rid, bool create = true)
  {
    byte_array::ConstByteArray const &address = rid.id();
    index_type                        index   = 0;

    std::lock_guard<mutex::Mutex> lock(mutex_);

    // If the file already exists, seek to it
    if (key_index_.GetIfExists(address, index))
    {
      file_object_.SeekFile(index);
    }
    else if (create)
    {
      // Else create
      file_object_.CreateNewFile();
    }
    else
    {
      Document document;
      document.failed = true;
      return document;
    }

    return file_object_.AsDocument();
  }

  Document Get(ResourceID const &rid)
  {
    return GetOrCreate(rid, false);
  }

  void Set(ResourceID const &rid, byte_array::ConstByteArray const &value)
  {
    byte_array::ConstByteArray const &address = rid.id();
    index_type                        index   = 0;

    std::lock_guard<mutex::Mutex> lock(mutex_);

    if (key_index_.GetIfExists(address, index))
    {
      file_object_.SeekFile(index);
    }
    else
    {
      // Create new file, with new index etc.
      // write this to the key index
      file_object_.CreateNewFile(value.size());
    }

    file_object_.Resize(value.size());

    file_object_.Write(value);
    file_object_.Flush();

    key_index_.Set(address, file_object_.id(), file_object_.Hash());
    key_index_.Flush();
  }

  void Erase(ResourceID const &rid)
  {
    byte_array::ConstByteArray const &address = rid.id();
    index_type                        index   = 0;

    std::lock_guard<mutex::Mutex> lock(mutex_);

    if (key_index_.GetIfExists(address, index))
    {
      file_object_.SeekFile(index);
    }
    else
    {
      return;
    }

    key_index_.Erase(address);
    key_index_.Flush();

    file_object_.Erase();
    file_object_.Flush();
  }

  void Flush(bool lazy = true)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    file_object_.Flush(lazy);
    key_index_.Flush(lazy);
  }

  std::size_t size() const
  {
    return key_index_.size();
  }

  /**
   * STL-like functionality achieved with an iterator class. This has to wrap an
   * iterator to the
   * key value store since we need to deserialize at this level to return the
   * object
   */
  class Iterator
  {
  public:
    Iterator(self_type *self, typename key_value_index_type::Iterator it)
      : wrapped_iterator_{it}
      , self_{self}
    {}

    Iterator()                    = default;
    Iterator(Iterator const &rhs) = default;
    Iterator(Iterator &&rhs)      = default;
    Iterator &operator=(Iterator const &rhs) = default;
    Iterator &operator=(Iterator &&rhs) = default;

    void operator++()
    {
      ++wrapped_iterator_;
    }

    bool operator==(Iterator const &rhs) const
    {
      return wrapped_iterator_ == rhs.wrapped_iterator_;
    }

    bool operator!=(Iterator const &rhs) const
    {
      return !(wrapped_iterator_ == rhs.wrapped_iterator_);
    }

    Document operator*() const
    {
      auto kv = *wrapped_iterator_;

      // The key value (index of file) must be valid at this point
      self_->file_object_.SeekFile(kv.second);

      return self_->file_object_.AsDocument();
    }

  protected:
    typename key_value_index_type::Iterator wrapped_iterator_;
    self_type *                             self_;
  };

  self_type::Iterator Find(ResourceID const &rid)
  {
    byte_array::ConstByteArray const &address = rid.id();
    auto                              it      = key_index_.Find(address);

    return Iterator(this, it);
  }

  /**
   * Get an iterator to the first element of a subtree (the first element of the
   * range that
   * matches the first bits of rid)
   *
   * @param: rid The key
   * @param: bits The number of bits of rid we want to match against
   *
   * @return: an iterator to the first element of that tree
   */
  self_type::Iterator GetSubtree(ResourceID const &rid, uint64_t bits)
  {
    byte_array::ConstByteArray const &address = rid.id();
    auto                              it      = key_index_.GetSubtree(address, bits);

    return Iterator(this, it);
  }

  self_type::Iterator begin()
  {
    return Iterator(this, key_index_.begin());
  }

  self_type::Iterator end()
  {
    return Iterator(this, key_index_.end());
  }

  // Hash based functionality - note this will only work if both underlying files
  // have commit functionality
  byte_array_type Commit()
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    byte_array_type               hash = key_index_.Hash();

    if (key_index_.underlying_stack().HashExists(hash) ||
        file_object_.underlying_stack().HashExists(hash))
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Attempted to commit an already committed hash");
      return hash;
    }

    key_index_.underlying_stack().Commit(hash);
    file_object_.underlying_stack().Commit(hash);

    return hash;
  }

  bool RevertToHash(byte_array_type const &hash)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);

    // TODO(private issue 615): HashExists implement
    if (!(key_index_.underlying_stack().HashExists(hash) &&
          file_object_.underlying_stack().HashExists(hash)))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Attempted to revert to a hash that doesn't exist");
      return false;
    }

    key_index_.underlying_stack().RevertToHash(hash);
    file_object_.underlying_stack().RevertToHash(hash);

    key_index_.UpdateVariables();
    file_object_.UpdateVariables();

    return true;
  }

  bool HashExists(byte_array_type const &hash)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    return key_index_.underlying_stack().HashExists(hash) &&
           file_object_.underlying_stack().HashExists(hash);
  }

  hash_type CurrentHash()
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    return key_index_.Hash();
  }

protected:
  mutex::Mutex         mutex_{__LINE__, __FILE__};
  key_value_index_type key_index_;
  file_object_type     file_object_;
};

}  // namespace storage
}  // namespace fetch
