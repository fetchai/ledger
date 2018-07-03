#ifndef STORAGE_INDEXED_DOCUMENT_STORE_HPP
#define STORAGE_INDEXED_DOCUMENT_STORE_HPP
#include <cassert>
#include <fstream>
#include <memory>
#include "core/byte_array/referenced_byte_array.hpp"
#include "storage/file_object.hpp"
#include "storage/key_value_index.hpp"
#include "storage/resource_mapper.hpp"
#include "network/service/protocol.hpp"
namespace fetch {
namespace storage {


template< std::size_t BS = 2048, typename A =  FileBlockType<BS>, typename B = KeyValueIndex<>,  typename C = VersionedRandomAccessStack< A >, typename D = FileObject<C>  >
class DocumentStore {
public:
  enum {
    BLOCK_SIZE = BS
  };
  
  typedef DocumentStore<BS,A,B,C,D> self_type;
  typedef byte_array::ByteArray byte_array_type;  

  typedef A file_block_type;
  typedef B key_value_index_type;
  typedef C file_store_type;
  typedef D file_object_type;
/*
 typedef FileBlockType<BLOCK_SIZE> file_block_type;
  typedef KeyValueIndex<> key_value_index_type;
  typedef VersionedRandomAccessStack< file_block_type >  file_store_type;
  typedef FileObject<file_store_type> file_object_type;
*/  
  typedef typename key_value_index_type::index_type index_type;

  class DocumentImplementation : public file_object_type
  {
  public:
    DocumentImplementation(self_type *s, byte_array::ConstByteArray const &address, file_store_type &store)
      : file_object_type(store), address_(address), store_(s)
    {  }
    
    DocumentImplementation(self_type *s, byte_array::ConstByteArray const &address, file_store_type &store, std::size_t const&pos )
      : file_object_type(store, pos), address_(address), store_(s)
    {  }
    
    ~DocumentImplementation() 
    {
      store_->UpdateDocument( *this );
    }
    DocumentImplementation(DocumentImplementation const& other) = delete;  
    DocumentImplementation operator=(DocumentImplementation const& other) = delete;  

    
    DocumentImplementation(DocumentImplementation && other) = default;  
    DocumentImplementation &operator=(DocumentImplementation && other) = default;  

    byte_array::ConstByteArray const &address() const
    {
      return address_;
    }
    
  private:
    byte_array::ConstByteArray address_;
    self_type *store_;    
  };
  
  class Document
  {
  public:
    Document(self_type *s, byte_array::ConstByteArray const &address,
      file_store_type &store)
    {
      pointer_ = std::make_shared< DocumentImplementation >(s, address, store);      
    }
    
    Document(self_type *s, byte_array::ConstByteArray const &address,
      file_store_type &store, std::size_t const&pos )
    {
      pointer_ = std::make_shared< DocumentImplementation >(s, address, store, pos);      
    }

    uint64_t Tell() {
      return pointer_->Tell();
    }

    void Seek(uint64_t const &n) {
      pointer_->Seek(n);
    }
    
    
    void Read(byte_array::ByteArray &arr) 
    {
      pointer_->Read(arr);
    }
  
    void Read(uint8_t *bytes, uint64_t const &m)
    {
      pointer_->Read(bytes, m);
    }

    void Shrink(uint64_t const &m)
    {
      pointer_->Shrink(m);
    }
    
    
    void Write(byte_array::ConstByteArray const &arr)
    {
      pointer_->Write(arr);
    }
    
    void Write(uint8_t const * bytes, uint64_t const &m) 
    {
      pointer_->Write(bytes, m);
    }
    
    std::size_t id() const 
    {
      return pointer_->id();
    }
    
    std::size_t size() const 
    {
      return pointer_->size();
    }
    
    byte_array::ConstByteArray const &address() const
    {
      return pointer_->address();
    }

  private:
    std::shared_ptr< DocumentImplementation > pointer_;
  };


  void Load(std::string const &doc_file, std::string const &doc_diff,
    std::string const &index_file, std::string const &index_diff, bool const &create = true) 
  {
    file_store_.Load(doc_file, doc_diff, create);    
    key_index_.Load(index_file, index_diff, create); 
  }

  void New(std::string const &doc_file, std::string const &doc_diff,
    std::string const &index_file, std::string const &index_diff)  
  {
    file_store_.New(doc_file, doc_diff);    
    key_index_.New(index_file, index_diff); 
  }

  void Load(std::string const &doc_file,
    std::string const &index_file, bool const &create = true) 
  {
    file_store_.Load(doc_file, create);    
    key_index_.Load(index_file,  create); 
  }

  void New(std::string const &doc_file, 
    std::string const &index_file)  
  {
    file_store_.New(doc_file);    
    key_index_.New(index_file); 
  }
    
  Document GetDocumentBuffer(ResourceID const &rid, bool const &create = true)
  {
    byte_array::ConstByteArray const &address = rid.id();    
    index_type index = 0 ;
    
    
    if(key_index_.GetIfExists(address, index)) {
      std::cout << "Get 1: " << index << std::endl;
      
      return Document(this, address, file_store_, index);
    } else if(!create) {
      TODO_FAIL("TODO: Throw error");
    }
    std::cout << "Get 2" << std::endl;
    Document doc = Document(this, address, file_store_);

    return doc;
  }

  byte_array::ConstByteArray Get(ResourceID const &rid) 
  {
    Document doc = GetDocumentBuffer(rid, false);
    byte_array::ByteArray ret;
    ret.Resize( doc.size() );
    doc.Read(ret);
    return ret;
  }

  void Set(ResourceID const &rid, byte_array::ConstByteArray const& value) 
  {
    Document doc = GetDocumentBuffer(rid, true);
    doc.Seek(0);
    
    if(doc.size() > value.size() ) {
      doc.Shrink(value.size());
    }
    
    doc.Write(value);

  }
  
  
  byte_array::ConstByteArray Hash() 
  {
    return key_index_.Hash();
  }

  // TODO
  typedef uint64_t bookmark_type;
//  bookmark_type Commit() {
//    return key_index_.Commit();
//  }

  bookmark_type Commit(bookmark_type const& b) {
    file_store_.Commit(b);    
    return key_index_.Commit(b);
  }  
  
  void Revert(bookmark_type const &b) {
    std::cout << "First revert" << std::endl;    
    file_store_.Revert(b);
    std::cout << "Second revert" << std::endl;
    
    key_index_.Revert(b);
  }
  
private:  
  void UpdateDocument(DocumentImplementation &doc)
  {
    std::cout << "Flushing and closing" << std::endl;
    doc.Flush();
    key_index_.Set(doc.address(), doc.id(), doc.Hash());
    
//TODO:    file_store_.Flush();
    key_index_.Flush();    
  }
  

  key_value_index_type key_index_;
  file_store_type file_store_;

};


// TODO: make more readible
typedef DocumentStore<2048,  FileBlockType<2048>, KeyValueIndex<>,  VersionedRandomAccessStack<  FileBlockType<2048> >, FileObject< VersionedRandomAccessStack<  FileBlockType<2048> > > > RevertibleDocumentStore;


}
}

#endif
