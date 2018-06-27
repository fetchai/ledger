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
  
  class Document : public file_object_type 
  {
  public:
    Document(IndexedDocumentStore *s, file_store_type &store)
      : file_object_type(store), store_(s)
    {  }
    
    Document(IndexedDocumentStore *s, file_store_type &store, std::size_t const&pos )
      : file_object_type(store, pos), store_(s)
    {  }

    Document(Document const& other) = delete;  
    Document operator=(Document const& other) = delete;  
    
    Document(Document && other) = default;  
    Document &operator=(Document && other) = default;  
    
    ~Document( ) 
    {
      store_->UpdateDocument( *this );
    }
    
  private:
    IndexedDocumentStore *store_;
    
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
    index_type index = index_type(-1);
    
    if(key_index_.GetIfExists(address, index)) {
      std::cout << "Get 1: " << index << std::endl;
      
      return Document(this, file_store_, index);
    } else if(!create) {
      TODO_FAIL("TODO: Throw error");
    }
    std::cout << "Get 2" << std::endl;
    return Document(this, file_store_);
//    return doc;    

  }

private:  
  void UpdateDocument(Document &doc)
  {
    std::cout << "Flushing and closing" << std::endl;
    
    
  }
  

  key_value_index_type key_index_;
  file_store_type file_store_;

};
}
}

#endif
