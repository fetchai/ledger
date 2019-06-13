#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"
#include "vm/common.hpp"

namespace fetch {
namespace vm {

using ByteArrayBuffer = fetch::serializers::ByteArrayBuffer;

// Forward declarations
class Object;
template <typename T>
class Ptr;
struct Variant;
class Address;
struct String;

template <typename T, typename = void>
struct IsPrimitive : std::false_type
{
};
template <typename T>
struct IsPrimitive<
    T, std::enable_if_t<type_util::IsAnyOfV<T, void, bool, int8_t, uint8_t, int16_t, uint16_t,
                                            int32_t, uint32_t, int64_t, uint64_t, float, double>>>
  : std::true_type
{
};

template <typename T, typename R = void>
using IfIsPrimitive = typename std::enable_if<IsPrimitive<std::decay_t<T>>::value, R>::type;

template <typename T, typename = void>
struct IsObject : std::false_type
{
};
template <typename T>
struct IsObject<T, typename std::enable_if_t<std::is_base_of<Object, T>::value>> : std::true_type
{
};

template <typename T>
struct IsPtr : std::false_type
{
};
template <typename T>
struct IsPtr<Ptr<T>> : std::true_type
{
};

template <typename T, typename R = void>
using IfIsPtr = typename std::enable_if<IsPtr<std::decay_t<T>>::value, R>::type;

template <typename T, typename = void>
struct IsVariant : std::false_type
{
};
template <typename T>
struct IsVariant<T, typename std::enable_if_t<std::is_base_of<Variant, T>::value>> : std::true_type
{
};

template <typename T, typename = void>
struct IsAddress : std::false_type
{
};
template <typename T>
struct IsAddress<T, typename std::enable_if_t<std::is_base_of<Address, T>::value>> : std::true_type
{
};

template <typename T, typename = void>
struct IsString : std::false_type
{
};
template <typename T>
struct IsString<T, std::enable_if_t<std::is_base_of<String, std::decay_t<T>>::value>>
  : std::true_type
{
};

template <typename T, typename = void>
struct IsNonconstRef : std::false_type
{
};
template <typename T>
struct IsNonconstRef<
    T, typename std::enable_if_t<std::is_same<T, typename std::decay<T>::type &>::value>>
  : std::true_type
{
};

template <typename T, typename = void>
struct IsConstRef : std::false_type
{
};
template <typename T>
struct IsConstRef<
    T, typename std::enable_if_t<std::is_same<T, typename std::decay<T>::type const &>::value>>
  : std::true_type
{
};

template <typename T>
struct GetManagedType;
template <typename T>
struct GetManagedType<Ptr<T>>
{
  using type = T;
};

template <typename T, typename = void>
struct IsPrimitiveParameter : std::false_type
{
};
template <typename T>
struct IsPrimitiveParameter<
    T, typename std::enable_if_t<!IsNonconstRef<T>::value &&
                                 IsPrimitive<typename std::decay<T>::type>::value>> : std::true_type
{
};

template <typename T, typename = void>
struct IsPtrParameter : std::false_type
{
};
template <typename T>
struct IsPtrParameter<T, typename std::enable_if_t<IsConstRef<T>::value &&
                                                   IsPtr<typename std::decay<T>::type>::value>>
  : std::true_type
{
};

template <typename T, typename = void>
struct IsVariantParameter : std::false_type
{
};
template <typename T>
struct IsVariantParameter<T,
                          typename std::enable_if_t<IsConstRef<T>::value &&
                                                    IsVariant<typename std::decay<T>::type>::value>>
  : std::true_type
{
};

template <typename T, typename = void>
struct GetStorageType;
template <typename T>
struct GetStorageType<T, typename std::enable_if_t<IsPrimitive<T>::value>>
{
  using type = T;
};
template <typename T>
struct GetStorageType<T, typename std::enable_if_t<IsPtr<T>::value>>
{
  using type = Ptr<Object>;
};

class Object
{
public:
  Object()          = delete;
  virtual ~Object() = default;

  Object(VM *vm, TypeId type_id)
    : vm_(vm)
    , type_id_(type_id)
    , ref_count_(1)
  {}

  virtual size_t GetHashCode();
  virtual bool   IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual bool   IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual bool   IsLessThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual bool   IsLessThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual bool   IsGreaterThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual bool   IsGreaterThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual void   Negate(Ptr<Object> &object);
  virtual void   Add(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void   LeftAdd(Variant &lhsv, Variant &objectv);
  virtual void   RightAdd(Variant &objectv, Variant &rhsv);
  virtual void   InplaceAdd(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual void   InplaceRightAdd(Ptr<Object> const &lhso, Variant const &rhsv);
  virtual void   Subtract(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void   LeftSubtract(Variant &lhsv, Variant &objectv);
  virtual void   RightSubtract(Variant &objectv, Variant &rhsv);
  virtual void   InplaceSubtract(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual void   InplaceRightSubtract(Ptr<Object> const &lhso, Variant const &rhsv);
  virtual void   Multiply(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void   LeftMultiply(Variant &lhsv, Variant &objectv);
  virtual void   RightMultiply(Variant &objectv, Variant &rhsv);
  virtual void   InplaceMultiply(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual void   InplaceRightMultiply(Ptr<Object> const &lhso, Variant const &rhsv);
  virtual void   Divide(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void   LeftDivide(Variant &lhsv, Variant &objectv);
  virtual void   RightDivide(Variant &objectv, Variant &rhsv);
  virtual void   InplaceDivide(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual void   InplaceRightDivide(Ptr<Object> const &lhso, Variant const &rhsv);

  virtual bool SerializeTo(ByteArrayBuffer &buffer);
  virtual bool DeserializeFrom(ByteArrayBuffer &buffer);

  TypeId GetTypeId() const
  {
    return type_id_;
  }

protected:
  Variant &       Push();
  Variant &       Pop();
  Variant &       Top();
  void            RuntimeError(std::string const &message);
  TypeInfo const &GetTypeInfo(TypeId type_id);
  bool            GetNonNegativeInteger(Variant const &v, size_t &index);

  VM *   vm_;
  TypeId type_id_;
  size_t ref_count_;

private:
  void AddRef()
  {
    ++ref_count_;
  }

  void Release()
  {
    if (--ref_count_ == 0)
    {
      delete this;
    }
  }

  template <typename T>
  friend class Ptr;
};

template <typename T>
class Ptr
{
public:
  Ptr()
  {
    ptr_ = nullptr;
  }

  Ptr(T *other)
  {
    ptr_ = other;
  }

  static Ptr PtrFromThis(T *this__)
  {
    this__->AddRef();
    return Ptr(this__);
  }

  Ptr &operator=(std::nullptr_t /* other */)
  {
    Reset();
    return *this;
  }

  Ptr(Ptr const &other)
  {
    ptr_ = other.ptr_;
    AddRef();
  }

  Ptr(Ptr &&other)
  {
    ptr_       = other.ptr_;
    other.ptr_ = nullptr;
  }

  template <typename U>
  Ptr(Ptr<U> const &other)
  {
    ptr_ = static_cast<T *>(other.ptr_);
    AddRef();
  }

  template <typename U>
  Ptr(Ptr<U> &&other)
  {
    ptr_       = static_cast<T *>(other.ptr_);
    other.ptr_ = nullptr;
  }

  Ptr &operator=(Ptr const &other)
  {
    if (ptr_ != other.ptr_)
    {
      Release();
      ptr_ = other.ptr_;
      AddRef();
    }
    return *this;
  }

  Ptr &operator=(Ptr &&other)
  {
    if (this != &other)
    {
      Release();
      ptr_       = other.ptr_;
      other.ptr_ = nullptr;
    }
    return *this;
  }

  template <typename U>
  Ptr &operator=(Ptr<U> const &other)
  {
    if (ptr_ != other.ptr_)
    {
      Release();
      ptr_ = static_cast<T *>(other.ptr_);
      AddRef();
    }
    return *this;
  }

  template <typename U>
  Ptr &operator=(Ptr<U> &&other)
  {
    Release();
    ptr_       = static_cast<T *>(other.ptr_);
    other.ptr_ = nullptr;
    return *this;
  }

  ~Ptr()
  {
    Release();
  }

  void Reset()
  {
    if (ptr_)
    {
      ptr_->Release();
      ptr_ = nullptr;
    }
  }

  explicit operator bool() const
  {
    return ptr_ != nullptr;
  }

  T *operator->() const
  {
    return ptr_;
  }

  T &operator*() const
  {
    return *ptr_;
  }

  size_t RefCount() const
  {
    return ptr_->ref_count_;
  }

private:
  T *ptr_;

  void AddRef()
  {
    if (ptr_)
    {
      ptr_->AddRef();
    }
  }

  void Release()
  {
    if (ptr_)
    {
      ptr_->Release();
    }
  }

  template <typename U>
  friend class Ptr;

  template <typename L, typename R>
  friend bool operator==(Ptr<L> const &lhs, Ptr<R> const &rhs);

  template <typename L>
  friend bool operator==(Ptr<L> const &lhs, std::nullptr_t /* rhs */);

  template <typename R>
  friend bool operator==(std::nullptr_t /* lhs */, Ptr<R> const &rhs);

  template <typename L, typename R>
  friend bool operator!=(Ptr<L> const &lhs, Ptr<R> const &rhs);

  template <typename L>
  friend bool operator!=(Ptr<L> const &lhs, std::nullptr_t /* rhs */);

  template <typename R>
  friend bool operator!=(std::nullptr_t /* lhs */, Ptr<R> const &rhs);
};

template <typename L, typename R>
inline bool operator==(Ptr<L> const &lhs, Ptr<R> const &rhs)
{
  return (lhs.ptr_ == static_cast<L *>(rhs.ptr_));
}

template <typename L>
inline bool operator==(Ptr<L> const &lhs, std::nullptr_t /* rhs */)
{
  return (lhs.ptr_ == nullptr);
}

template <typename R>
inline bool operator==(std::nullptr_t /* lhs */, Ptr<R> const &rhs)
{
  return (nullptr == rhs.ptr_);
}

template <typename L, typename R>
inline bool operator!=(Ptr<L> const &lhs, Ptr<R> const &rhs)
{
  return (lhs.ptr_ != static_cast<L *>(rhs.ptr_));
}

template <typename L>
inline bool operator!=(Ptr<L> const &lhs, std::nullptr_t /* rhs */)
{
  return (lhs.ptr_ != nullptr);
}

template <typename R>
inline bool operator!=(std::nullptr_t /* lhs */, Ptr<R> const &rhs)
{
  return (nullptr != rhs.ptr_);
}

inline void Serialize(ByteArrayBuffer &buffer, Ptr<Object> const &object)
{
  if (!object)
  {
    throw std::runtime_error("Unable to serialize null reference");
  }

  if (!object->SerializeTo(buffer))
  {
    throw std::runtime_error("Unable to serialize requested object");
  }
}

inline void Deserialize(ByteArrayBuffer &buffer, Ptr<Object> &object)
{
  if (!object)
  {
    throw std::runtime_error("Unable to deserialize in to null reference object");
  }

  // TODO (issue 1172): This won't work in general (not for nested Ptr<Object> types (e.g. `Array`
  // of String, Array of Array, etc ...).
  //                    Current serialisation principle firmly requires types to be `default
  //                    constructable`, reason being that in order for types to be
  //                    deserializable they need to have default constructor (= non-parametric
  //                    constructor).
  //                    It is necessary to extend/change current serialisation concept
  //                    and enable default-like construction of VM `Object` based types.
  if (!object->DeserializeFrom(buffer))
  {
    throw std::runtime_error("Object deserialisation failed.");
  }
}

class Unknown;

template <template <typename T, typename... Args> class Functor, typename... Args>
inline auto TypeIdAsCanonicalType(TypeId const type_id, Args &&... args)
{
  switch (type_id)
  {
  case TypeIds::Unknown:
    return Functor<Unknown>{}(std::forward<Args>(args)...);

  case TypeIds::Null:
    return Functor<std::nullptr_t>{}(std::forward<Args>(args)...);

  case TypeIds::Void:
    return Functor<void>{}(std::forward<Args>(args)...);

  case TypeIds::Bool:
    return Functor<uint8_t>{}(std::forward<Args>(args)...);

  case TypeIds::Int8:
    return Functor<int8_t>{}(std::forward<Args>(args)...);

  case TypeIds::UInt8:
    return Functor<uint8_t>{}(std::forward<Args>(args)...);

  case TypeIds::Int16:
    return Functor<int16_t>{}(std::forward<Args>(args)...);

  case TypeIds::UInt16:
    return Functor<uint16_t>{}(std::forward<Args>(args)...);

  case TypeIds::Int32:
    return Functor<int32_t>{}(std::forward<Args>(args)...);

  case TypeIds::UInt32:
    return Functor<uint32_t>{}(std::forward<Args>(args)...);

  case TypeIds::Int64:
    return Functor<int64_t>{}(std::forward<Args>(args)...);

  case TypeIds::UInt64:
    return Functor<uint64_t>{}(std::forward<Args>(args)...);

  case TypeIds::Float32:
    return Functor<float>{}(std::forward<Args>(args)...);

  case TypeIds::Float64:
    return Functor<double>{}(std::forward<Args>(args)...);

  case TypeIds::String:
    return Functor<Ptr<String>>{}(std::forward<Args>(args)...);

  case TypeIds::Address:
    return Functor<Ptr<Address>>{}(std::forward<Args>(args)...);

  default:
    return Functor<Ptr<Object>>{}(std::forward<Args>(args)...);
  }  // switch
}

}  // namespace vm
}  // namespace fetch
