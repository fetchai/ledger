#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/assert.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/meta/type_traits.hpp"
#include "vectorise/memory/shared_array.hpp"

#include <initializer_list>
#include <memory>
#include <ostream>
#include <type_traits>
#include <vector>

namespace fetch {
namespace script {

enum VariantType
{
  UNDEFINED      = 0,
  INTEGER        = 1,
  FLOATING_POINT = 2,
  BOOLEAN        = 3,
  STRING         = 4,
  NULL_VALUE     = 5,
  ARRAY          = 6,
  OBJECT         = 7
};

class Variant;
class VariantArray;
class VariantProxy;

class Variant
{
public:
  using ConstByteArray = byte_array::ConstByteArray;

  // Construction / Destruction
  Variant();
  Variant(int64_t const &i);
  Variant(int32_t const &i);
  Variant(int16_t const &i);
  Variant(uint64_t const &i);
  Variant(uint32_t const &i);
  Variant(uint16_t const &i);
  Variant(float const &f);
  Variant(double const &f);
  explicit Variant(ConstByteArray const &o)
  {
    *this = o;
  }
  Variant(std::initializer_list<Variant> const &lst);
  ~Variant() = default;

  void ReleaseResources();

  // Type creations
  void MakeNull();
  void MakeUndefined();
  void MakeArray(std::size_t n);
  void MakeObject();

  // Helpers
  static Variant Array(std::size_t n);
  static Variant Object();

  // Assignment
  template <typename T>
  meta::IfIsIntegerLike<T, Variant &> operator=(T const &i);

  template <typename T>
  meta::IfIsFloatLike<T, Variant &> operator=(T const &f);

  template <typename T>
  meta::IfIsBooleanLike<T, Variant &> operator=(T const &b);

  Variant &operator=(VariantArray const &array);
  Variant &operator=(ConstByteArray const &b);
  Variant &operator=(char const *data);

  // Dict accessors
  VariantProxy   operator[](ConstByteArray const &key);
  Variant const &operator[](ConstByteArray const &key) const;

  // Array accessors
  Variant &      operator[](std::size_t const &i);
  Variant const &operator[](std::size_t const &i) const;
  std::size_t    size() const;

  bool Append(ConstByteArray const &key, Variant const &val);
  void SetArray(VariantArray const &data, std::size_t offset, std::size_t size);
  void SetObject(VariantArray const &data, std::size_t offset, std::size_t size);

  template <typename... A>
  void EmplaceSetString(A... args);

  template <typename T>
  meta::IfIsIntegerLike<T, T> As() const;

  template <typename T>
  meta::IfIsBooleanLike<T, const T &> As() const;

  template <typename T>
  meta::IfIsFloatLike<T, T> As() const;

  template <typename T>
  meta::IfIsByteArrayLike<T, const T &> As() const;

  template <typename T>
  meta::IfIsStdStringLike<T, T> As() const;

  bool is_int() const
  {
    return type_ == INTEGER;
  }
  bool is_float() const
  {
    return type_ == FLOATING_POINT;
  }
  bool is_bool() const
  {
    return type_ == BOOLEAN;
  }
  bool is_array() const
  {
    return type_ == ARRAY;
  }
  bool is_object() const
  {
    return type_ == OBJECT;
  }
  bool is_string() const
  {
    return type_ == STRING;
  }
  bool is_byte_array() const
  {
    return type_ == STRING;
  }
  bool is_undefined() const
  {
    return type_ == UNDEFINED;
  }

  ConstByteArray const &as_byte_array() const
  {
    return string_;
  }

  VariantType type() const
  {
    return type_;
  }

  friend std::ostream &operator<<(std::ostream &os, Variant const &v);

  void ForEach(std::function<bool(Variant const &key, Variant &value)> const &object_functor);
  void ForEach(
      std::function<bool(Variant const &key, Variant const &value)> const &object_functor) const;
  void ForEach(std::function<bool(Variant &value)> const &object_functor);
  void ForEach(std::function<bool(Variant const &value)> const &object_functor) const;

private:
  using VariantArrayPtr = std::shared_ptr<VariantArray>;

  std::size_t FindKeyIndex(byte_array::ConstByteArray const &key) const;
  void        LazyAppend(byte_array::ConstByteArray const &key, Variant const &val);

  union PrimitiveData
  {
    int64_t integer;
    double  float_point;
    bool    boolean;
  };

  PrimitiveData         data_;
  byte_array::ByteArray string_;
  VariantArrayPtr       array_ = std::make_shared<VariantArray>();
  VariantType           type_  = UNDEFINED;

  friend VariantProxy;
};

class VariantProxy : public Variant
{
public:
  using ConstByteArray = byte_array::ConstByteArray;

  VariantProxy(ConstByteArray key, Variant *parent)
    : key_(std::move(key))
    , parent_(parent)
    , child_(nullptr)
  {}

  VariantProxy(ConstByteArray key, Variant *parent, Variant *child)
    : Variant(*child)
    , key_(std::move(key))
    , parent_(parent)
    , child_(child)
  {}

  ~VariantProxy()
  {
    if (modified_)
    {
      if (child_ != nullptr)
      {
        child_->operator=(*this);
      }
      else
      {
        parent_->LazyAppend(key_, *this);
      }
    }
  }

  template <typename S>
  S operator=(S val)
  {
    modified_        = true;
    Variant::operator=(val);
    return val;
  }

private:
  ConstByteArray key_;
  Variant *      parent_   = nullptr;
  Variant *      child_    = nullptr;
  bool           modified_ = false;
};

class VariantArray
{
public:
  VariantArray() = default;
  VariantArray(std::size_t const &size);
  VariantArray(VariantArray const &other, std::size_t offset, std::size_t size);
  VariantArray(VariantArray const &other) = default;
  VariantArray(VariantArray &&other)      = default;

  VariantArray &operator=(VariantArray const &other) = default;
  VariantArray &operator=(VariantArray &&other) noexcept = default;

  ~VariantArray() = default;

  Variant const &operator[](std::size_t const &i) const;
  Variant &      operator[](std::size_t const &i);
  void           Resize(std::size_t const &n);
  void           Reserve(std::size_t const &n);
  std::size_t    size() const
  {
    return size_;
  }

  // Circular references can form when constructing variants in that any variant in this subarray
  // can point back to this subarray, causing a memory leak. To break this, we force every variant
  // in our list to release its resources. This will not play well if the variant array is not
  // wholly contained (but for our use case it is)
  void ReleaseResources()
  {
    if (data_)
    {
      for (auto &variant : *data_)
      {
        variant.ReleaseResources();
      }
    }
  }

  void SetData(VariantArray const &other, std::size_t offset, std::size_t size);

  template <typename FROM_ITERATOR>
  VariantArray &CopyFrom(FROM_ITERATOR const &from_start_itr, FROM_ITERATOR const &from_end_itr,
                         std::size_t offset = 0)
  {
    auto const        dist      = std::distance(from_start_itr, from_end_itr);
    std::size_t const from_size = static_cast<std::size_t>(dist);
    if (offset + from_size > size())
    {
      Resize(offset + from_size);
    }

    FROM_ITERATOR start = from_start_itr;
    FROM_ITERATOR end   = from_end_itr;
    if (dist < 0)
    {
      start = from_end_itr;
      end   = from_start_itr;
    }

    std::copy(start, end, pointer_ + offset);
    return *this;
  }

  void ForEach(std::function<bool(Variant const &key, Variant &value)> const &object_functor)
  {
    assert(0 == (size() & 1));  //* even

    if (!object_functor || size() == 0)
    {
      return;
    }

    for (std::size_t key_idx = 0, val_idx = key_idx + 1; key_idx < size();
         key_idx += 2, val_idx += 2)
    {
      if (!object_functor((*this)[key_idx], (*this)[val_idx]))
      {
        break;
      }
    }
  }

  void ForEach(std::function<bool(Variant &value)> const &object_functor)
  {
    if (!object_functor)
    {
      return;
    }

    for (std::size_t val_idx = 0; val_idx < size(); ++val_idx)
    {
      if (!object_functor((*this)[val_idx]))
      {
        break;
      }
    }
  }

private:
  using Container    = std::vector<Variant>;
  using ContainerPtr = std::shared_ptr<Container>;

  std::size_t  size_   = 0;
  std::size_t  offset_ = 0;
  ContainerPtr data_;
  Variant *    pointer_ = nullptr;
};

inline Variant::Variant()
  : type_(UNDEFINED)
{}

inline Variant::Variant(int64_t const &i)
{
  *this = i;
}
inline Variant::Variant(int32_t const &i)
{
  *this = i;
}
inline Variant::Variant(int16_t const &i)
{
  *this = i;
}
inline Variant::Variant(uint64_t const &i)
{
  *this = i;
}
inline Variant::Variant(uint32_t const &i)
{
  *this = i;
}
inline Variant::Variant(uint16_t const &i)
{
  *this = i;
}
inline Variant::Variant(float const &f)
{
  *this = f;
}
inline Variant::Variant(double const &f)
{
  *this = f;
}

inline void Variant::ReleaseResources()
{
  array_.reset();
}

inline void Variant::MakeNull()
{
  type_ = NULL_VALUE;
}
inline void Variant::MakeUndefined()
{
  type_ = UNDEFINED;
}

inline void Variant::MakeArray(std::size_t n)
{
  type_   = ARRAY;
  *array_ = VariantArray(n);
}

inline void Variant::MakeObject()
{
  type_   = OBJECT;
  *array_ = VariantArray();
}

inline Variant Variant::Array(std::size_t n)
{
  Variant ret;
  ret.MakeArray(n);
  return ret;
}

inline Variant Variant::Object()
{
  Variant ret;
  ret.MakeObject();
  return ret;
}

inline Variant &Variant::operator=(ConstByteArray const &b)
{
  type_   = STRING;
  string_ = b;
  return *this;
}

template <typename T>
meta::IfIsIntegerLike<T, Variant &> Variant::operator=(T const &i)
{
  type_         = INTEGER;
  data_.integer = int64_t(i);
  return *this;
}

template <typename T>
meta::IfIsFloatLike<T, Variant &> Variant::operator=(T const &f)
{
  type_             = FLOATING_POINT;
  data_.float_point = double(f);
  return *this;
}

template <typename T>
meta::IfIsBooleanLike<T, Variant &> Variant::operator=(T const &b)
{
  type_         = BOOLEAN;
  data_.boolean = b;
  return *this;
}

inline Variant &Variant::operator=(VariantArray const &array)
{
  type_   = ARRAY;
  *array_ = array;
  return *this;
}

template <typename... A>
void Variant::EmplaceSetString(A... args)
{
  type_ = STRING;
  string_.FromByteArray(args...);
}

template <typename T>
meta::IfIsIntegerLike<T> Variant::As() const
{
  assert(type_ == INTEGER);
  return static_cast<T>(data_.integer);
}

template <typename T>
meta::IfIsBooleanLike<T, const T &> Variant::As() const
{
  assert(type_ == BOOLEAN);
  return data_.boolean;
}

template <typename T>
meta::IfIsFloatLike<T, T> Variant::As() const
{
  assert(type_ == FLOATING_POINT);
  return static_cast<T>(data_.float_point);
}

template <typename T>
meta::IfIsByteArrayLike<T, const T &> Variant::As() const
{
  assert(type_ == STRING);
  return string_;
}

template <typename T>
meta::IfIsStdStringLike<T, T> Variant::As() const
{
  assert(type_ == STRING);
  return static_cast<std::string>(string_);
}

inline std::ostream &operator<<(std::ostream &os, Variant const &v)
{
  switch (v.type())
  {
  case VariantType::UNDEFINED:
    os << "(undefined)";
    break;
  case VariantType::INTEGER:
    os << v.As<int64_t>();
    break;
  case VariantType::FLOATING_POINT:
    os << v.As<double>();
    break;
  case VariantType::STRING:
    os << '"' << v.as_byte_array() << '"';
    break;
  case VariantType::BOOLEAN:
    os << (v.As<bool>() ? "true" : "false");
    break;
  case VariantType::NULL_VALUE:
    os << "null";
    break;
  case VariantType::ARRAY:
    os << "[";

    for (std::size_t i = 0; i < v.array_->size(); ++i)
    {
      if (i != 0)
      {
        os << ", ";
      }
      os << (*v.array_)[i];
    }

    os << "]";

    break;
  case VariantType::OBJECT:
    os << "{";
    for (std::size_t i = 0; i < v.array_->size(); i += 2)
    {
      if (i != 0)
      {
        os << ", ";
      }
      os << (*v.array_)[i] << ": " << (*v.array_)[i + 1];
    }

    os << "}";

    break;
  }
  return os;
}

inline std::ostream &operator<<(std::ostream &os, VariantArray const &v)
{
  os << "[";
  for (std::size_t i = 0; i < v.size(); ++i)
  {
    if (i != 0)
    {
      os << ", ";
    }
    os << v[i];
  }
  os << "]";

  return os;
}

template <typename T>
inline bool Extract(script::Variant const &obj, byte_array::ConstByteArray const &name, T &value)
{
  auto element = obj[name];
  if (element.is_undefined())
  {
    return false;
  }

  value = element.As<T>();
  return true;
}

inline void Variant::ForEach(
    std::function<bool(Variant const &key, Variant &value)> const &object_functor)
{
  assert(type_ == OBJECT);
  array_->ForEach(object_functor);
}

inline void Variant::ForEach(
    std::function<bool(Variant const &key, Variant const &value)> const &object_functor) const
{
  assert(type_ == OBJECT);
  array_->ForEach(object_functor);
}

inline void Variant::ForEach(std::function<bool(Variant &value)> const &object_functor)
{
  assert(type_ == ARRAY);
  array_->ForEach(object_functor);
}

inline void Variant::ForEach(std::function<bool(Variant const &value)> const &object_functor) const
{
  assert(type_ == ARRAY);
  array_->ForEach(object_functor);
}

}  // namespace script
}  // namespace fetch
