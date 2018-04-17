#ifndef SCRIP_VARIANT_HPP
#define SCRIP_VARIANT_HPP
#include "byte_array/referenced_byte_array.hpp"
#include "memory/shared_array.hpp"
//#include "script/dictionary.hpp"

#include <initializer_list>
#include <ostream>
#include <type_traits>
#include <vector>
namespace fetch {
namespace script {
enum VariantType {
  UNDEFINED = 0,
  INTEGER = 1,
  FLOATING_POINT = 2,
  BOOLEAN = 3,
  STRING = 4,
  NULL_VALUE = 5,
  ARRAY = 6,
  OBJECT = 7
};


  class Variant;
  
class VariantList 
{
public:
  VariantList() ;
  VariantList(std::size_t const& size ) ;
  VariantList(VariantList const& other, std::size_t offset, std::size_t size)  ;
  VariantList(VariantList const& other);
  VariantList(VariantList && other) ;

  VariantList const &operator=(VariantList const& other);
  VariantList const &operator=(VariantList && other) ;

  Variant const& operator[](std::size_t const &i)  const;  
  Variant & operator[](std::size_t const &i);
  void Resize(std::size_t const &n) ;
  void Reserve(std::size_t const &n);
  std::size_t size() const { return size_; }

  void SetData(VariantList const& other, std::size_t offset, std::size_t size) ;
private:
  std::size_t size_ = 0;
  std::size_t offset_ = 0;
  memory::SharedArray< Variant, 16 > data_;
  Variant *pointer_ = nullptr;
}; 

class Variant
{  
public:
  typedef byte_array::ByteArray byte_array_type;
  typedef VariantList  variant_array_type;  
  
  
  Variant() : type_(UNDEFINED) {}
  
  Variant(int64_t const& i) { *this = i; }
  Variant(int32_t const& i) { *this = i; }
  Variant(int16_t const& i) { *this = i; }
  Variant(uint64_t const& i) { *this = i; }
  Variant(uint32_t const& i) { *this = i; }
  Variant(uint16_t const& i) { *this = i; }

  Variant(float const& f) { *this = f; }
  Variant(double const& f) { *this = f; }
  
  Variant(std::initializer_list< Variant > const & lst) {    
    type_ = ARRAY;
    VariantList data(lst.size());
    std::size_t i = 0;
    for(auto const &a: lst)
      data[i++] = a;

    array_ = data;
  }
  
  ~Variant() 
  {
    
  }

  void MakeNull() {
    type_ = NULL_VALUE;
  }

  void MakeUndefined() {
    type_ = UNDEFINED;
  }

  void MakeArray(std::size_t n) 
  {
    type_ = ARRAY;
    array_ = VariantList(n);
  }

  void MakeObject() 
  {
    type_ = OBJECT;
    array_ = VariantList();
  }
  
  static Variant Array(std::size_t n) 
  {
    Variant ret;
    ret.MakeArray(n);    
    return ret;    
  }

  static Variant Object() 
  {
    Variant ret;
    ret.MakeObject();
    return ret;    
  }  
  
  
  byte_array_type const& operator=(byte_array_type const &b) {
    type_ = STRING;
    string_ = b;
    return b;
  }

  char const* operator=(char* const data) {
    if(data == nullptr)
      type_ = NULL_VALUE;
    else {
      type_ = STRING;
      string_ = data;
    }
    
    return data;
  }  
  
  
  template <typename T>
  typename std::enable_if< (!std::is_same<T,bool>::value) && std::is_integral<T>::value, T>::type operator=(
      T const& i) {
    type_ = INTEGER;
    return data_.integer = i;
  }

  template <typename T>
  typename std::enable_if<std::is_floating_point<T>::value, T>::type operator=(
      T const& f) {
    type_ = FLOATING_POINT;
    return data_.float_point = f;
  }

  template <typename T>
  typename std::enable_if<std::is_same<T, bool>::value, T>::type   
  operator=(T const& b) {
    type_ = BOOLEAN;
    return data_.boolean = b;
  }


  variant_array_type const& operator=(variant_array_type const& array) {
    type_ = ARRAY;
    array_ = array;
    return array;
  }


  // Dict accessors
  Variant & operator[](byte_array::BasicByteArray const &key) 
  {
    assert(type_ == OBJECT);
    std::size_t i = 0;
    for(; i < array_.size(); i += 2) {
      if(key == array_[i].as_byte_array()) break;
    }
    if(i == array_.size()) {
      // TODO: Add add functionality
    }
    return array_[i+1];
  }

  Variant const & operator[](byte_array::BasicByteArray const &key) const 
  {
    assert(type_ == OBJECT);
    std::size_t i = 0;
    for(; i < array_.size(); i += 2) {
      if(key == array_[i].as_byte_array()) break;
    }
    if(i == array_.size()) {
      // TODO: Throw error
    }
    return array_[i+1];    
  }

  // Array accessors
  Variant& operator[](std::size_t const& i);  
  Variant const& operator[](std::size_t const& i) const;   
  std::size_t size() const;
  
  
  void SetArray(VariantList const& data, std::size_t offset, std::size_t size)  {
    type_ = ARRAY;
    array_.SetData(data, offset, size) ;
  }

  void SetObject(VariantList const& data, std::size_t offset, std::size_t size)  {
    type_ = OBJECT;
    array_.SetData(data, offset, size) ;
  }
  
  template< typename ...A >
  void EmplaceSetString(A ...args ) {
    type_ = STRING;
    string_.FromByteArray( args... );
  }

  
  int64_t const& as_int() const { return data_.integer; }
  int64_t& as_int() { return data_.integer; }
  double const& as_double() const { return data_.float_point; }
  double& as_double() { return data_.float_point; }
  bool const& as_bool() const { return data_.boolean; }
  bool& as_bool() { return data_.boolean; }

  byte_array_type const& as_byte_array() const { return string_; }
  byte_array_type& as_byte_array() { return string_; }

  variant_array_type const& as_array() const { return array_; }
  variant_array_type & as_array() { return array_; }  


  VariantType type() const 
  {
    return type_;
  }
  
private:
  union {
    int64_t integer;
    double float_point;
    bool boolean;
  } data_;
  
  byte_array::ByteArray string_;
  variant_array_type array_;
  
  VariantType type_ = UNDEFINED;  
};



  VariantList::VariantList() : size_(0) 
  {
    pointer_ = data_.pointer();    
  }

  VariantList::VariantList(std::size_t const& size ) 
  {
    Resize(size);
  }  

  VariantList::VariantList(VariantList const& other, std::size_t offset, std::size_t size) :
    size_(size), offset_(offset), data_(other.data_)
  {
    pointer_ = data_.pointer() + offset_;
  }  

  VariantList::VariantList(VariantList const& other) :
    size_(other.size_), offset_(other.offset_), data_(other.data_), pointer_(other.pointer_)
  {
  }

  VariantList::VariantList(VariantList && other) 
  {
    std::swap(size_,other.size_);
    std::swap(offset_,other.offset_);
    std::swap(data_,other.data_);
    std::swap(pointer_,other.pointer_);
  }

VariantList const &VariantList::operator=(VariantList const& other) {
  size_ = other.size_;
  offset_ = other.offset_;
  data_ = other.data_;
  pointer_=other.pointer_;
  return *this;
}
VariantList const &VariantList::operator=(VariantList && other) {
  std::swap(size_,other.size_);
  std::swap(offset_,other.offset_);
  std::swap(data_,other.data_);
  std::swap(pointer_,other.pointer_);
  return *this;  
}

  
  Variant const& VariantList::operator[](std::size_t const &i)  const
  {
    return pointer_[i];
  }
  
Variant & VariantList::operator[](std::size_t const &i) 
{
  return pointer_[i];
}

void VariantList::Resize(std::size_t const &n) 
{
  if(size_ == n) return;
  Reserve(n);
  size_ = n;
  }

  void VariantList::Reserve(std::size_t const &n) 
  {
    if(offset_ + n < data_.size() ) return;

    memory::SharedArray< Variant, 16 > new_data(n);
    new_data.SetAllZero(); // TODO: Ugly hack to prevent conditional jump

    data_ = new_data;
    offset_ = 0;    
    pointer_ = data_.pointer();
  }
    

void VariantList::SetData(VariantList const& other, std::size_t offset, std::size_t size)  {
  data_ = other.data_;
  size_ = size;
  offset_ = offset;
  pointer_ = data_.pointer() + offset;
  }

// Array accessors
Variant& Variant::operator[](std::size_t const& i) {
  assert(type_ == ARRAY);
  assert(i < size());
  return array_[i];    
}

Variant const& Variant::operator[](std::size_t const& i) const {
  return array_[i];
}
  
  
std::size_t Variant::size() const {
  if (type_ == ARRAY)
    return array_.size();
  if (type_ == STRING) return string_.size();
  return 0;
}


std::ostream& operator<<(std::ostream& os, Variant const& v) {

  switch (v.type()) {
    case VariantType::UNDEFINED:
      os << "(undefined)";
      break;
    case VariantType::INTEGER:
      os << v.as_int();
      break;
    case VariantType::FLOATING_POINT:
      os << v.as_double();
      break;
    case VariantType::STRING:
      os << '"' << v.as_byte_array() << '"';
      break;
    case VariantType::BOOLEAN:
      os << (v.as_bool() ? "true" : "false");
      break;
    case VariantType::NULL_VALUE:
      os << "null";
      break;
    case VariantType::ARRAY:
      os << "[";
      
      for(std::size_t i=0; i < v.as_array().size(); ++i) {
        if(i != 0) {
          os << ", ";
        }
        os << v.as_array()[i];

      }
      
      os << "]";
      
      break;
    case VariantType::OBJECT:
      os << "{";
      for(std::size_t i=0; i < v.as_array().size(); ++i) {
        if(i != 0) {
          os << ", ";
        }
        os <<  v.as_array()[i++] << ": " << v.as_array()[i];
      }      

      os << "}";
      
      break;            
  }
  return os;  
}



std::ostream& operator<<(std::ostream& os, VariantList const& v) {

  os << "[";
  for(std::size_t i=0; i < v.size(); ++i) {
    if(i != 0) {
      os << ", ";
    }
    os << v[i];    
  }
  os << "]";
  
  return os;  
}


};
};
#endif
