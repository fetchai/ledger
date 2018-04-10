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
  BYTE_ARRAY = 3,
  BOOLEAN = 4,
  NULL_VALUE = 5,
  ARRAY = 6,
  DICTIONARY = 7,
  FUNCTION = 8
};

// FIXME: replace asserts with throwing errors
  class Variant;
  std::ostream& operator<<(std::ostream& os, Variant const& v);  

class Variant {
public:
  typedef fetch::byte_array::ByteArray byte_array_type;
  typedef fetch::memory::SharedArray<Variant> variant_array_type;
  typedef fetch::script::Dictionary<Variant> variant_dictionary_type;
  
  Variant() : type_(UNDEFINED) {}
  Variant(int64_t const& i) { *this = i; }
  Variant(int32_t const& i) { *this = i; }
  Variant(int16_t const& i) { *this = i; }
  Variant(uint64_t const& i) { *this = i; }
  Variant(uint32_t const& i) { *this = i; }
  Variant(uint16_t const& i) { *this = i; }

  Variant(float const& f) { *this = f; }
  Variant(double const& f) { *this = f; }

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

  std::nullptr_t* operator=(std::nullptr_t* ptr) {
    FreeMemory();
    if (ptr != nullptr) {
      // FIXME: Throw error
      assert(ptr == nullptr);
    }
    type_ = NULL_VALUE;
    return ptr;
  }

  void SetArrayPointer(Variant *ptr)
  {
    FreeMemory();
    
  }

  void SetObjectPointer(Variant *ptr)
  {
    FreeMemory();
    
  }
  

  
  
  template <typename T>
  typename std::enable_if<std::is_integral<T>::value, T>::type operator=(
      T const& i) {
    FreeMemory();
    type_ = INTEGER;
    return data_.integer = i;
  }

  template <typename T>
  typename std::enable_if<std::is_floating_point<T>::value, T>::type operator=(
      T const& f) {
    FreeMemory();
    type_ = FLOATING_POINT;
    return data_.float_point = f;
  }

  bool operator=(bool const& b) {
    FreeMemory();
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

  byte_array_type const & operator=(byte_array_type const& s) {
    FreeMemory();
    type_ = BYTE_ARRAY;
//    std::cout << "Was here???" << std::endl;
    byte_array_ = s;
    return byte_array_;
   
  }

  byte_array_type const& as_byte_array() const { return byte_array_; }
  byte_array_type& as_byte_array() { return byte_array_; }
  int64_t const& as_int() const { return data_.integer; }
  int64_t& as_int() { return data_.integer; }
  double const& as_double() const { return data_.float_point; }
  double& as_double() { return data_.float_point; }
  bool const& as_bool() const { return data_.boolean; }
  bool& as_bool() { return data_.boolean; }


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


  union {
    int64_t integer;
    double float_point;
    bool boolean;
    variant_array_type* array;
    variant_dictionary_type *object;
  } data_;
  
  byte_array_type byte_array_;
  VariantType type_ = UNDEFINED;
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
};
};
#endif
