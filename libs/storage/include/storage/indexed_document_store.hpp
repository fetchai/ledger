#ifndef STORAGE_INDEXED_DOCUMENT_STORE_HPP
#define STORAGE_INDEXED_DOCUMENT_STORE_HPP
#include <cassert>
#include <fstream>
#include <memory>
#include "core/byte_array/referenced_byte_array.hpp"
#include "storage/file_object.hpp"
#include "storage/key_value_index.hpp"
namespace fetch {
namespace storage {

class IndexedDocumentStore {
 public:
  typedef byte_array::ByteArray byte_array_type;  
  
  enum {
    BLOCK_SIZE = 2048
  };
  
  typedef FileBlockType<BLOCK_SIZE> file_block_type;
  typedef KeyValueIndex<> key_value_index_type;
  typedef VersionedRandomAccessStack< file_block_type >  file_store_type;
  typedef FileObject<file_store_type> file_object_type;

  typedef typename key_value_index_type::index_type index_type;

  class DocumentImplementation : public file_object_type
  {
  public:
    DocumentImplementation(IndexedDocumentStore *s, byte_array::ConstByteArray const &address, file_store_type &store)
      : file_object_type(store), address_(address), store_(s)
    {  }
    
    DocumentImplementation(IndexedDocumentStore *s, byte_array::ConstByteArray const &address, file_store_type &store, std::size_t const&pos )
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
    IndexedDocumentStore *store_;    
  };
  
    
  
  class Document
  {
  public:
    Document(IndexedDocumentStore *s, byte_array::ConstByteArray const &address,
      file_store_type &store)
    {
      pointer_ = std::make_shared< DocumentImplementation >(s, address, store);      
    }
    
    Document(IndexedDocumentStore *s, byte_array::ConstByteArray const &address,
      file_store_type &store, std::size_t const&pos )
    {
      pointer_ = std::make_shared< DocumentImplementation >(s, address, store, pos);      
    }

    std::size_t id() const 
    {
      return pointer_->id();
    }
    
    byte_array::ConstByteArray const &address() const
    {
      return pointer_->address();
    }


  private:
    std::shared_ptr< DocumentImplementation > pointer_;
    
  };
  
  void Load(std::string const &doc_file, std::string const &doc_diff,
    std::string const &index_file, bool const &create = true) 
  {
    file_store_.Load(doc_file, doc_diff, create);    
    key_index_.Load(index_file, create); //, index_diff); , std::string const &index_diff
  }

  void New(std::string const &doc_file, std::string const &doc_diff,
    std::string const &index_file)  // , std::string const &index_diff
  {
    file_store_.New(doc_file, doc_diff);    
    key_index_.New(index_file); // , index_diff
  }


  Document GetDocument(byte_array::ConstByteArray const &address, bool const &create = true)
  {
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

  byte_array::ConstByteArray Hash() 
  {
    return key_index_.Hash();
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
}
}

#endif
