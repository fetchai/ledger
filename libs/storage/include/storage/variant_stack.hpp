#ifndef STORAGE_VARIANT_STACK_HPP
#define STORAGE_VARIANT_STACK_HPP

#include <cassert>
#include <fstream>
#include <string>
#include "core/assert.hpp"

// TODO: Make variant stack as a circular buffer!

namespace fetch {
namespace storage {

class VariantStack {
 public:
  struct Separator {
    uint64_t type;
    uint64_t object_size;
    int64_t previous;
    Separator() 
    {
      memset(this, 0, sizeof(decltype(*this)));
    }
    Separator(uint64_t const& t, uint64_t const& o, int64_t const& p) 
    {
      memset(this, 0, sizeof(decltype(*this))); 
      type = t;
      object_size = o;
      previous = p;     
    }
    
  };

  struct Header {
    uint64_t object_count;
    int64_t end;
    
    Header(uint64_t const &o, int64_t const &e) 
    {
      memset(this, 0, sizeof(decltype(*this)));      
      object_count = o;
      end = e;      
    }
    
    Header() 
    {
      memset(this, 0, sizeof(decltype(*this)));
      end = sizeof(decltype(*this)) + sizeof(Separator);
    }   
  };
  
  ~VariantStack() 
  {
    Close();
  }
  
  void Load(std::string const &filename) {
    filename_ = filename;
    file_ = std::fstream(filename_,
      std::ios::in | std::ios::out | std::ios::binary);
    ReadHeader();
  }

  void New(std::string const &filename) {
    filename_ = filename;
    Clear();
    file_ = std::fstream(filename_,
      std::ios::in | std::ios::out | std::ios::binary);
    assert(bool(file_));
    
  }
  
  void Close() 
  {
    WriteHeader();
    if(file_.is_open()) {
      file_.close();
    }
  }

  enum { UNDEFINED_POSITION =  int64_t(-1) };
  enum { HEADER_OBJECT = uint64_t(-2) };
  
  template <typename T>
  void Push(T const &object, uint64_t const &type = uint64_t(-1)) {
    assert( bool(file_) );    
    file_.seekg(header_.end, file_.beg);
    Separator separator = {type, sizeof(T),  header_.end };

    file_.write(reinterpret_cast<char const *>(&object), sizeof(T));
    file_.write(reinterpret_cast<char const *>(&separator), sizeof(Separator));
    header_.end += sizeof(T) + sizeof(Separator);
    ++header_.object_count;
//    WriteHeader();    
  }

  void Pop() {
    file_.seekg(header_.end - int64_t(sizeof(Separator)), file_.beg);
    Separator separator;

    file_.read(reinterpret_cast<char *>(&separator), sizeof(Separator));

    header_.end = separator.previous;
    --header_.object_count;
//    WriteHeader();    
  }

  template <typename T>
  uint64_t Top(T &object) {
    assert( bool(file_) );

    file_.seekg(header_.end - int64_t(sizeof(Separator)), file_.beg);
    Separator separator;

    file_.read(reinterpret_cast<char *>(&separator), sizeof(Separator));
    int64_t offset = int64_t(sizeof(Separator) + separator.object_size);
    
    if (separator.object_size != sizeof(T)) {
      std::cout << offset << " " << separator.object_size << " " << sizeof(T) << std::endl;
      
      TODO_FAIL("TODO: Throw error, size mismatch in variantstack");
    }

    file_.seekg(header_.end - offset, file_.beg); 
    file_.read(reinterpret_cast<char *>(&object), sizeof(T));
    return separator.type;
  }

  uint64_t Type() {
    file_.seekg(header_.end - int64_t(sizeof(Separator)), file_.beg);
    Separator separator;

    file_.read(reinterpret_cast<char *>(&separator), sizeof(Separator));
    
    return separator.type;
  }

  void Clear() {
    assert( bool(file_) );        
    assert(filename_ != "");
    std::fstream fin(filename_, std::ios::out | std::ios::binary);
    fin.seekg(0, fin.beg);
    
    Separator separator = {HEADER_OBJECT, 0, UNDEFINED_POSITION} ;
    
    header_ = Header();
    fin.write(reinterpret_cast<char const *>(&header_), sizeof(Header));
    fin.write(reinterpret_cast<char const *>(&separator), sizeof(Separator));
      
    fin.close();
  }

  bool empty() const { return header_.object_count == 0; }
  int64_t size() const { return int64_t(header_.object_count); }
  protected: 
  void ReadHeader() 
  {
    file_.seekg(0, file_.beg);
    file_.read(reinterpret_cast<char *>(&header_), sizeof(Header));
  }

  void WriteHeader() 
  {
    file_.seekg(0, file_.beg);    
    file_.write(reinterpret_cast<char const *>(&header_), sizeof(Header));    
  }
  
  
 private:  
  std::fstream file_;  
  std::string filename_ = "";
  Header header_;
};
}
}

#endif
