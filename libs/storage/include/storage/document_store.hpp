#pragma once
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

template <std::size_t BS = 2048, typename A = FileBlockType<BS>, typename B = KeyValueIndex<>,
          typename C = VersionedRandomAccessStack<A>, typename D = FileObject<C>>
class DocumentStore
{
public:
  enum
  {
    BLOCK_SIZE = BS
  };

  typedef DocumentStore<BS, A, B, C, D> self_type;
  typedef byte_array::ByteArray         byte_array_type;

  typedef A file_block_type;
  typedef B key_value_index_type;
  typedef C file_store_type;
  typedef D file_object_type;

  typedef byte_array::ConstByteArray hash_type;

  typedef typename key_value_index_type::index_type index_type;

  class DocumentFileImplementation : public file_object_type
  {
  public:
    DocumentFileImplementation(self_type *s, byte_array::ConstByteArray const &address,
                               file_store_type &store)
        : file_object_type(store), address_(address), store_(s)
    {}

    DocumentFileImplementation(self_type *s, byte_array::ConstByteArray const &address,
                               file_store_type &store, std::size_t const &pos)
        : file_object_type(store, pos), address_(address), store_(s)
    {}

    ~DocumentFileImplementation() { store_->UpdateDocumentFile(*this); }
    DocumentFileImplementation(DocumentFileImplementation const &other) = delete;
    DocumentFileImplementation operator=(DocumentFileImplementation const &other) = delete;

    DocumentFileImplementation(DocumentFileImplementation &&other) = default;
    DocumentFileImplementation &operator=(DocumentFileImplementation &&other) = default;

    byte_array::ConstByteArray const &address() const { return address_; }

    self_type const *store() const { return store_; }

  private:
    byte_array::ConstByteArray address_;
    self_type *                store_;
  };

  class DocumentFile
  {
  public:
    operator bool() const { return static_cast<bool>(pointer_); }

    DocumentFile() : pointer_(nullptr) {}

    DocumentFile(self_type *s, byte_array::ConstByteArray const &address, file_store_type &store)
    {
      pointer_     = std::make_shared<DocumentFileImplementation>(s, address, store);
      was_created_ = true;
    }

    DocumentFile(self_type *s, byte_array::ConstByteArray const &address, file_store_type &store,
                 std::size_t const &pos)
    {
      pointer_ = std::make_shared<DocumentFileImplementation>(s, address, store, pos);
    }

    uint64_t Tell() { return pointer_->Tell(); }

    void Seek(uint64_t const &n) { pointer_->Seek(n); }

    void Read(byte_array::ByteArray &arr) { pointer_->Read(arr); }

    void Read(uint8_t *bytes, uint64_t const &m) { pointer_->Read(bytes, m); }

    void Shrink(uint64_t const &m) { pointer_->Shrink(m); }

    void Write(byte_array::ConstByteArray const &arr) { pointer_->Write(arr); }

    void Write(uint8_t const *bytes, uint64_t const &m) { pointer_->Write(bytes, m); }

    std::size_t id() const { return pointer_->id(); }

    std::size_t size() const { return pointer_->size(); }

    byte_array::ConstByteArray const &address() const { return pointer_->address(); }

    bool was_created() const { return was_created_; }

  private:
    std::shared_ptr<DocumentFileImplementation> pointer_;
    bool                                        was_created_ = false;
  };

  void Load(std::string const &doc_file, std::string const &doc_diff, std::string const &index_file,
            std::string const &index_diff, bool const &create = true)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    file_store_.Load(doc_file, doc_diff, create);
    key_index_.Load(index_file, index_diff, create);
  }

  void New(std::string const &doc_file, std::string const &doc_diff, std::string const &index_file,
           std::string const &index_diff)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    file_store_.New(doc_file, doc_diff);
    key_index_.New(index_file, index_diff);
  }

  void Load(std::string const &doc_file, std::string const &index_file, bool const &create = true)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    file_store_.Load(doc_file, create);
    key_index_.Load(index_file, create);
  }

  void New(std::string const &doc_file, std::string const &index_file)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    file_store_.New(doc_file);
    key_index_.New(index_file);
  }

  Document GetOrCreate(ResourceID const &rid)
  {
    Document ret;

    std::lock_guard<mutex::Mutex> lock(mutex_);
    DocumentFile                  doc = GetDocumentFile(rid, true);

    if (!doc)
    {
      ret.failed = true;
    }
    else
    {

      ret.was_created = doc.was_created();
      if (!doc.was_created())
      {

        ret.document.Resize(doc.size());
        doc.Read(ret.document);
      }
    }

    return ret;
  }

  Document Get(ResourceID const &rid)
  {
    Document ret;

    std::lock_guard<mutex::Mutex> lock(mutex_);
    DocumentFile                  doc = GetDocumentFile(rid, false);
    if (!doc)
    {
      ret.failed = true;
    }
    else
    {

      ret.was_created = doc.was_created();
      if (!doc.was_created())
      {

        ret.document.Resize(doc.size());
        doc.Read(ret.document);
      }
    }

    return ret;
  }

  void Set(ResourceID const &rid, byte_array::ConstByteArray const &value)
  {
    std::lock_guard<mutex::Mutex> lock(mutex_);
    DocumentFile                  doc = GetDocumentFile(rid, true);
    doc.Seek(0);

    if (doc.size() > value.size())
    {
      doc.Shrink(value.size());
    }

    doc.Write(value);
  }

protected:
  DocumentFile GetDocumentFile(ResourceID const &rid, bool const &create = true)
  {
    byte_array::ConstByteArray const &address = rid.id();
    index_type                        index   = 0;

    if (key_index_.GetIfExists(address, index))
    {
      return DocumentFile(this, address, file_store_, index);
    }
    else if (!create)
    {

      return DocumentFile();
    }

    DocumentFile doc = DocumentFile(this, address, file_store_);

    return doc;
  }

  mutex::Mutex         mutex_{ __LINE__, __FILE__ };
  key_value_index_type key_index_;
  file_store_type      file_store_;

private:
  void UpdateDocumentFile(DocumentFileImplementation &doc)
  {

    doc.Flush();
    key_index_.Set(doc.address(), doc.id(), doc.Hash());

    // TODO:    file_store_.Flush();
    key_index_.Flush();
  }
};

}  // namespace storage
}  // namespace fetch
