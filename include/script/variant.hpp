#ifndef SCRIP_VARIANT_HPP
#define SCRIP_VARIANT_HPP
#include "byte_array/referenced_byte_array.hpp"
#include "memory/shared_array.hpp"
#include "script/dictionary.hpp"

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



class VariantList;

class VariantAtomic
{  
public:
  typedef byte_array::ByteArray byte_array_type;
  typedef VariantList  variant_array_type;  
  typedef std::shared_ptr< VariantList > shared_variant_array_type;
  typedef Dictionary< VariantAtomic > dictionary_type;
  
  
  VariantAtomic() : type_(UNDEFINED) {}
  
  VariantAtomic(int64_t const& i) { *this = i; }
  VariantAtomic(int32_t const& i) { *this = i; }
  VariantAtomic(int16_t const& i) { *this = i; }
  VariantAtomic(uint64_t const& i) { *this = i; }
  VariantAtomic(uint32_t const& i) { *this = i; }
  VariantAtomic(uint16_t const& i) { *this = i; }

  VariantAtomic(float const& f) { *this = f; }
  VariantAtomic(double const& f) { *this = f; }
  
  ~VariantAtomic() 
  {
    
  }

  std::nullptr_t* operator=(std::nullptr_t* ptr) {
    if (ptr != nullptr) {
      // FIXME: Throw error
      assert(ptr == nullptr);
    }
    type_ = NULL_VALUE;
    return ptr;
  }

  byte_array_type const& operator=(byte_array_type const &b) {
    type_ = STRING;
    string_ = b;
    return b;
  }
  
  
  template <typename T>
  typename std::enable_if<std::is_integral<T>::value, T>::type operator=(
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

  bool operator=(bool const& b) {
    type_ = BOOLEAN;
    return data_.boolean = b;
  }

  char const& operator=(char const& c) {
    byte_array_type str;
    str.Resize(1);
    str[0] = c;
    *this = str;
    return c;
  }


  variant_array_type const& operator=(variant_array_type const& array) {
    type_ = ARRAY;
    array_ = std::make_shared< variant_array_type >(array);
    return array;
  }


  // Dict accessors
  VariantAtomic & operator[](byte_array::BasicByteArray const &key) 
  {
    assert(type_ == OBJECT);
    return object_[key];
  }

  VariantAtomic const & operator[](byte_array::BasicByteArray const &key) const 
  {
    assert(type_ == OBJECT);
    return object_[key];
  }


  // Array accessors
  VariantAtomic& operator[](std::size_t const& i);  
  VariantAtomic const& operator[](std::size_t const& i) const;   
  std::size_t size() const;
  



  
  template< typename ...A >
  void EmplaceSetArray(A ...args ) {
    type_ = ARRAY;
    array_ = std::make_shared< variant_array_type >( args... );
  }

  template< typename ...A >
  void EmplaceSetString(A ...args ) {
    type_ = STRING;
    string_.FromByteArray( args... );
  }

  void MakeEmptyObject() 
  {
    type_ = OBJECT;
    object_.Clear();
  }
  
  
  int64_t const& as_int() const { return data_.integer; }
  int64_t& as_int() { return data_.integer; }
  double const& as_double() const { return data_.float_point; }
  double& as_double() { return data_.float_point; }
  bool const& as_bool() const { return data_.boolean; }
  bool& as_bool() { return data_.boolean; }

  byte_array_type const& as_byte_array() const { return string_; }
  byte_array_type& as_byte_array() { return string_; }

  variant_array_type const& as_array() const { return *array_; }
  variant_array_type & as_array() { return *array_; }  

  dictionary_type const& as_dictionary() const { return object_; }
  dictionary_type & as_dictionary() { return object_; }  
  

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
  shared_variant_array_type array_;
  dictionary_type object_;
  
  VariantType type_ = UNDEFINED;  
};


class VariantList 
{
public:
  VariantList() : size_(0) 
  {
    pointer_ = data_.pointer();    
  }

  VariantList(std::size_t const& size ) 
  {
    Resize(size);
  }  

  VariantList(VariantList const& other, std::size_t offset, std::size_t size) :
    size_(size), offset_(offset), data_(other.data_)
  {
    pointer_ = data_.pointer() + offset_;
  }  
  
  operator VariantAtomic&() 
  {
    return *pointer_;
  }
  
  VariantAtomic const& operator[](std::size_t const &i)  const
  {
    return pointer_[i];
  }
  
  VariantAtomic & operator[](std::size_t const &i) 
  {
    return pointer_[i];
  }

  void Resize(std::size_t const &n) 
  {
    if(size_ == n) return;
    Reserve(n);
    size_ = n;
  }

  void Reserve(std::size_t const &n) 
  {
    if(offset_ + n < data_.size() ) return;

    memory::SharedArray< VariantAtomic, 16 > new_data(n);

    /*
    for(std::size_t i = 0; i < data_.size(); ++i)
    {
      new_data[i] = pointer_[i];
    }    
    */
    data_ = new_data;
    offset_ = 0;    
    pointer_ = data_.pointer();
  }
  
  std::size_t size() const 
  {
    return size_;
  }

private:
  std::size_t size_ = 0;
  std::size_t offset_ = 0;
  memory::SharedArray< VariantAtomic, 16 > data_;
  VariantAtomic *pointer_ = nullptr;
};

    



// Array accessors
VariantAtomic& VariantAtomic::operator[](std::size_t const& i) {
  assert(type_ == ARRAY);
  assert(i < size());
  return (*array_)[i];    
}

VariantAtomic const& VariantAtomic::operator[](std::size_t const& i) const {
  return (*array_)[i];
}
  
  
std::size_t VariantAtomic::size() const {
  if (type_ == ARRAY)
    return array_->size();
  if (type_ == STRING) return string_.size();
  return 0;
}


std::ostream& operator<<(std::ostream& os, VariantAtomic const& v) {

  bool not_first = false;
  
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
      os << v.as_bool();
      break;
    case VariantType::NULL_VALUE:
      os << "(null)";
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
      for(auto &kv: v.as_dictionary() ) {
        if(not_first) {
          os << ", ";          
        }
        os << "\"" << kv.first << "\" : " <<  kv.second;
        not_first = true;
      }
      
/*      
      for(std::size_t i=0; i < v.as_array().size(); ++i) {
        if(i != 0) {
          os << ", ";
        }
        os << v.as_array()[i];

      }
*/      
      os << "}";
      
      break;            
  }
  return os;  
}


  


// FIXME: replace asserts with throwing errors
/*
  class Variant;
  std::ostream& operator<<(std::ostream& os, Variant const& v);  

class Variant {
public:
  typedef fetch::byte_array::ByteArray byte_array_type;
  typedef fetch::memory::SharedArray<Variant> variant_array_type;
  typedef fetch::script::Dictionary<Variant> variant_dictionary_type;
  

  Variant(char const* str) {
    if (str == nullptr)
      *this = nullptr;
    else
      *this = byte_array_type(str);
  }
  Variant(byte_array_type const& str) { *this = str; }
  Variant(char const& c) { *this = c; }

  Variant(std::initializer_list<Variant> const& arr) { *this = arr; }


  Variant(Variant const &var)
  {
    this->operator=(var);    
  }

  Variant(Variant &&var)
  {
    std::swap(data_, var.data_);
    std::swap(type_, var.type_);    
  }
  

  Variant const &operator=(Variant const &other)
  {
    
    type_ = other.type_;
    switch (other.type_) {
    case UNDEFINED:
    case BOOLEAN:      
    case INTEGER:
    case FLOATING_POINT:
    case NULL_VALUE:
      this->data_ = other.data_;
      
      break;
      
    case BYTE_ARRAY:
      this->byte_array_ = byte_array_ ;
      break;
      
    case ARRAY:
      this->data_.array = new variant_array_type(  *other.data_.array );        
      break;        
    case DICTIONARY:
      this->data_.object= new variant_dictionary_type(     *other.data_.object );
      break;
    case FUNCTION:
      // FIXME: implement
      break;
    }
    return other;    
  }
  

  
  static Variant Object(std::initializer_list<Variant> const& arr) {
    
    Variant ret;    
    ret.type_ = DICTIONARY;
    ret.data_.object = new variant_dictionary_type();
    for(auto const &kv: arr)
    {
      if(kv.size() != 2)
      {
        TODO_FAIL("Expected exactly two entries");        
      }

      ret[kv[0].as_byte_array()] =  kv[1]; 
    }
    return ret;    
  }

  static Variant Object() {
    Variant ret;    
    ret.type_ = DICTIONARY;
    ret.data_.object = new variant_dictionary_type();
    
    return ret;    
  }
  

  static Variant Array(std::initializer_list<Variant> const& arr) {
    Variant ret;    
    ret.type_ = ARRAY;
    ret.data_.array = new variant_array_type(arr.size());
    std::size_t i = 0;
    for(auto const &kv: arr)
    {      
      ret[i++] = kv;    
    }
    
    return ret;    
  }

  static Variant Array(std::size_t n = 0) {
    Variant ret;    
    ret.type_ = ARRAY;
    ret.data_.array = new variant_array_type(n);    
    return ret;    
  }  

  
  Variant Copy() const {
    Variant ret;
    std::cout << "Was here?" << std::endl;
    
    switch (type_) {
      case UNDEFINED:
      case INTEGER:
      case FLOATING_POINT:
      case BOOLEAN:
      case NULL_VALUE:
        ret = *this;
        break;

      case BYTE_ARRAY:
        ret.type_ = type_;
        ret.byte_array_ = byte_array_.Copy();
        break;

      case ARRAY:
        ret.type_ = type_;
        ret.data_.array = new variant_array_type(  data_.array->Copy() );        
        break;        
      case DICTIONARY:
        ret.type_ = type_;
        ret.data_.object= new variant_dictionary_type(     data_.object->Copy() );
        break;
      case FUNCTION:
        // FIXME: implement
        break;
    }
    return ret;
  }

  std::initializer_list<Variant> const& operator=(
      std::initializer_list<Variant> const& arr) {
    type_ = ARRAY;
    std::cout << "Was here??" << std::endl;    
    data_.array = new variant_array_type(arr.size());
    std::size_t i = 0;
    for (auto& a : arr) (*data_.array)[i++] = a;
    return arr;
  }

  
  // Array accessors
  Variant& operator[](std::size_t const& i) {
    assert(type_ == ARRAY);
    assert(i < size());
    return (*data_.array)[i];    
  }

  Variant const& operator[](std::size_t const& i) const {
    assert(type_ == ARRAY);
    assert(i < size());
    return (*data_.array)[i];
  }

  // Dict accessors
  Variant & operator[](byte_array::BasicByteArray const &key) 
  {
    assert(type_ == DICTIONARY);
    return (*data_.object)[key];
  }

  Variant const & operator[](byte_array::BasicByteArray const &key) const 
  {
    assert(type_ == DICTIONARY);
    return (*data_.object)[key];
  }

  
  std::size_t size() const {
    if (type_ == ARRAY)
      return data_.array->size();
    if (type_ == BYTE_ARRAY) return byte_array_.size();
    return 0;
  }


  void SetArrayPointer(Variant *ptr)
  {
    FreeMemory();
    
  }

  void SetObjectPointer(Variant *ptr)
  {
    FreeMemory();
    
  }
  

  
  

  byte_array_type const& as_byte_array() const { return byte_array_; }
  byte_array_type& as_byte_array() { return byte_array_; }


  variant_dictionary_type as_dictionary() { return *data_.object; }
  variant_dictionary_type const &as_dictionary() const { return *data_.object; }    
  
  variant_array_type as_array() { return *data_.array; }
  variant_array_type const &as_array() const { return *data_.array; }    

  bool is_null() const { return type_ == NULL_VALUE; }

  VariantType const& type() const { return type_; }

  bool operator==(char const* str) const {
    return (type_ == BYTE_ARRAY) && (as_byte_array() == str);
  }

  bool operator==(int const& v) const {
    return (type_ == INTEGER) && (as_int() == v);
  }

  bool operator==(double const& v) const {
    return (type_ == FLOATING_POINT) && (as_double() == v);
  }

  template <typename T>
  bool operator!=(T const& v) const {
    return !(*this == v);
  }

 private:
  void FreeMemory() {
    switch (type_) {
      case UNDEFINED:
      case INTEGER:
      case FLOATING_POINT:
      case BYTE_ARRAY:
        type_ = UNDEFINED;
        break;
      case BOOLEAN:
        break;
      case NULL_VALUE:
        break;
      case ARRAY:
        type_ = UNDEFINED;
        delete data_.array;
        data_.array = nullptr;
        break;        
      case DICTIONARY:
        type_ = UNDEFINED;
        delete data_.object;
        data_.object = nullptr;
        break;
      case FUNCTION:
        break;
    }
  }


  
  byte_array_type byte_array_;

};


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
    case VariantType::BYTE_ARRAY:
      os << '"' << v.as_byte_array() << '"';
      break;
    case VariantType::BOOLEAN:
      os << v.as_bool();
      break;
    case VariantType::NULL_VALUE:
      os << "(null)";
      break;
    case VariantType::ARRAY:
      os << "[";
      if (v.size() > 0) os << v[0];
      for (std::size_t i = 1; i < v.size(); ++i) {
        os << ", " << v[i];
      }
      os << "]";
      break;
  case VariantType::DICTIONARY: {
    bool first = true;
    
    os << "{";
    
    for(auto const &c: v.as_dictionary() )
    {
      
      if(!first)
        os << ", ";
      
      os << "\"" << c.first << "\": " << c.second;
      first = false;      
    }
    os << "}";
    
  }  
      break;
    case VariantType::FUNCTION:
      os << "(function)";
      break;
  }
  return os;
}
*/
};
};
#endif
