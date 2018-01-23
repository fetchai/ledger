#ifndef SCRIP_VARIANT_HPP
#define SCRIP_VARIANT_HPP
#include <byte_array/referenced_byte_array.hpp>
#include <memory/shared_array.hpp>

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


class Variant {

  class VariantAccessProxy {
  public:
    VariantAccessProxy(Variant& v, std::size_t const& i)
      : variant_(v), index_(i) {}
    
    operator Variant() {
      return (*variant_.data_.object_array)[index_];
    }
    //  Variant Copy() const;
    
    Variant const& operator=(Variant const& v) {
      assert((variant_.type() == VariantType::ARRAY) ||
             (variant_.type() == VariantType::DICTIONARY));
      assert(index_ < variant_.size());
      
      return (*variant_.data_.object_array)[index_ + 1] = v;
    }
    
    char operator=(char const& v) {
      assert(index_ < variant_.size());
      switch (variant_.type_) {
      case VariantType::BYTE_ARRAY:
        return (*variant_.data_.byte_array)[index_] = v;
      case VariantType::DICTIONARY:
      case VariantType::ARRAY:
        (*variant_.data_.object_array)[index_ + 1] = v;
        return v;
      default:
        assert(false);
        // TODO: throw error
        return 0;
      }
      return 0;
    }
    
    VariantType const& type() const {
      assert((variant_.type() == VariantType::ARRAY) ||
             (variant_.type() == VariantType::DICTIONARY));
      return (*variant_.data_.object_array)[index_ + 1].type();
      return variant_.type();
    }
    
  private:
    Variant& variant_;
    std::size_t index_;
  };
  

public:
  typedef fetch::byte_array::ByteArray byte_array_type;

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

  Variant Copy() const {
    Variant ret;
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
        ret.data_.byte_array = new byte_array_type(data_.byte_array->Copy());
        break;

      case ARRAY:
      case DICTIONARY:
        // FIXME: implement
        //      ret.data_.object_array = new variant_reference_type(
        //      data_.object_array->Copy() );
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
    data_.object_array = new variant_reference_type(arr.size() + 1);
    (*data_.object_array)[0] = arr.size();
    std::size_t i = 1;
    for (auto& a : arr) (*data_.object_array)[i++] = a;
    return arr;
  }

  VariantAccessProxy operator[](std::size_t const& i) {
    return VariantAccessProxy(*this, i);  //(*data_.object_array)[i+1];
  }

  Variant const& operator[](std::size_t const& i) const {
    assert(type_ == ARRAY);
    assert(i < (*data_.object_array)[0].as_int());
    return (*data_.object_array)[i + 1];
  }

  std::size_t size() const {
    if ((type_ == ARRAY) || (type_ == DICTIONARY))
      return (*data_.object_array)[0].as_int();
    if (type_ == BYTE_ARRAY) return data_.byte_array->size();
    return 0;
  }

  void* operator=(void* ptr) {
    FreeMemory();
    if (ptr != nullptr) {
      // FIXME: Throw error
      assert(ptr == nullptr);
    }
    type_ = NULL_VALUE;
    return ptr;
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

  byte_array_type operator=(byte_array_type const& s) {
    FreeMemory();
    type_ = BYTE_ARRAY;
    return *(data_.byte_array = new byte_array_type(s));
  }

  byte_array_type const& as_byte_array() const { return *data_.byte_array; }
  byte_array_type& as_byte_array() { return *data_.byte_array; }
  int64_t const& as_int() const { return data_.integer; }
  int64_t& as_int() { return data_.integer; }
  double const& as_double() const { return data_.float_point; }
  double& as_double() { return data_.float_point; }
  bool const& as_bool() const { return data_.boolean; }
  bool& as_bool() { return data_.boolean; }

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
        break;
      case INTEGER:
        break;
      case FLOATING_POINT:
        break;
      case BYTE_ARRAY:
        type_ = UNDEFINED;
        delete data_.byte_array;
        data_.byte_array = nullptr;
        break;
      case BOOLEAN:
        break;
      case NULL_VALUE:
        break;
      case ARRAY:
      case DICTIONARY:
        type_ = UNDEFINED;
        delete data_.object_array;
        data_.object_array = nullptr;
        break;
      case FUNCTION:
        break;
    }
  }

  typedef fetch::memory::SharedArray<Variant> variant_reference_type;
  union {
    int64_t integer;
    double float_point;
    bool boolean;
    byte_array_type* byte_array;
    variant_reference_type* object_array;
  } data_;

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
    case VariantType::DICTIONARY:
      os << "(object)";
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
