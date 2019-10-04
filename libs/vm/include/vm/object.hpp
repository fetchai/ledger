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

#include "core/serializers/base_types.hpp"
#include "core/serializers/main_serializer.hpp"
#include "variant/variant.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/common.hpp"

#include <type_traits>

namespace fetch {
namespace vm {

// TODO(issue 648): We should rename variants to VMVariant and JSONVariant, respectively
// to avoid name clash.
using JSONVariant       = fetch::variant::Variant;
using MsgPackSerializer = fetch::serializers::MsgPackSerializer;

// Forward declarations
class Object;
template <typename T>
class Ptr;
struct Variant;
class Address;
struct String;

template <typename T>
using IsPrimitive =
    type_util::IsAnyOf<T, void, bool, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
                       int64_t, uint64_t, float, double, fixed_point::fp32_t, fixed_point::fp64_t>;

template <typename T, typename R = void>
using IfIsPrimitive = std::enable_if_t<IsPrimitive<std::decay_t<T>>::value, R>;

template <typename T>
using IsObject = std::is_base_of<Object, T>;

template <typename T>
struct IsPtr : std::false_type
{
};
template <typename T>
struct IsPtr<Ptr<T>> : std::true_type
{
};

template <typename T, typename R = void>
using IfIsPtr = std::enable_if_t<IsPtr<std::decay_t<T>>::value, R>;

template <typename T>
using IsVariant = std::is_base_of<Variant, T>;

template <typename T>
using IsAddress = std::is_base_of<Address, T>;

template <typename T>
using IsString = std::is_base_of<String, std::decay_t<T>>;

template <typename T>
using IsNonconstRef = std::is_same<T, std::decay_t<T> &>;

template <typename T>
using IsConstRef = std::is_same<T, std::decay_t<T> const &>;

namespace internal {
template <typename T>
struct GetManagedTypeImpl;
template <typename T>
struct GetManagedTypeImpl<Ptr<T>>
{
  using type = T;
};
}  // namespace internal
template <typename T>
using GetManagedType = typename internal::GetManagedTypeImpl<T>::type;

template <typename T>
using IsPrimitiveParameter =
    std::integral_constant<bool, !IsNonconstRef<T>::value && IsPrimitive<std::decay_t<T>>::value>;

template <typename T>
using IsPtrParameter =
    std::integral_constant<bool, IsConstRef<T>::value && IsPtr<std::decay_t<T>>::value>;

template <typename T>
using IsVariantParameter =
    std::integral_constant<bool, IsConstRef<T>::value && IsVariant<std::decay_t<T>>::value>;

template <typename T, typename = void>
struct GetStorageType;
template <typename T>
struct GetStorageType<T, std::enable_if_t<IsPrimitive<T>::value>>
{
  using type = T;
};
template <typename T>
struct GetStorageType<T, std::enable_if_t<IsPtr<T>::value>>
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

  virtual std::size_t GetHashCode();
  virtual bool        IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual bool        IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual bool        IsLessThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual bool        IsLessThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual bool        IsGreaterThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual bool        IsGreaterThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual void        Negate(Ptr<Object> &object);
  virtual void        Add(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void        LeftAdd(Variant &lhsv, Variant &objectv);
  virtual void        RightAdd(Variant &objectv, Variant &rhsv);
  virtual void        InplaceAdd(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual void        InplaceRightAdd(Ptr<Object> const &lhso, Variant const &rhsv);
  virtual void        Subtract(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void        LeftSubtract(Variant &lhsv, Variant &objectv);
  virtual void        RightSubtract(Variant &objectv, Variant &rhsv);
  virtual void        InplaceSubtract(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual void        InplaceRightSubtract(Ptr<Object> const &lhso, Variant const &rhsv);
  virtual void        Multiply(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void        LeftMultiply(Variant &lhsv, Variant &objectv);
  virtual void        RightMultiply(Variant &objectv, Variant &rhsv);
  virtual void        InplaceMultiply(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual void        InplaceRightMultiply(Ptr<Object> const &lhso, Variant const &rhsv);
  virtual void        Divide(Ptr<Object> &lhso, Ptr<Object> &rhso);
  virtual void        LeftDivide(Variant &lhsv, Variant &objectv);
  virtual void        RightDivide(Variant &objectv, Variant &rhsv);
  virtual void        InplaceDivide(Ptr<Object> const &lhso, Ptr<Object> const &rhso);
  virtual void        InplaceRightDivide(Ptr<Object> const &lhso, Variant const &rhsv);

  virtual bool SerializeTo(MsgPackSerializer &buffer);
  virtual bool DeserializeFrom(MsgPackSerializer &buffer);

  virtual bool ToJSON(JSONVariant &variant);
  virtual bool FromJSON(JSONVariant const &variant);

  TypeId GetTypeId() const
  {
    return type_id_;
  }

  std::string GetUniqueId() const;

protected:
  Variant &       Push();
  Variant &       Pop();
  Variant &       Top();
  void            RuntimeError(std::string const &message);
  TypeInfo const &GetTypeInfo(TypeId type_id);
  bool            GetNonNegativeInteger(Variant const &v, std::size_t &index);

  VM *   vm_;
  TypeId type_id_;

private:
  std::size_t ref_count_;

  template <typename T>
  friend class Ptr;
};

template <typename T>
class Ptr
{
public:
  Ptr() = default;

  explicit Ptr(T *other) noexcept
    : ptr_{other}
  {}

  static Ptr PtrFromThis(T *this__)
  {
    auto ptr = Ptr(this__);
    ptr.AddRef();

    return ptr;
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

  constexpr Ptr(Ptr &&other) noexcept
  {
    ptr_       = other.ptr_;
    other.ptr_ = nullptr;
  }

  template <typename U>
  Ptr(Ptr<U> const &other)  // NOLINT
  {
    ptr_ = static_cast<T *>(other.ptr_);
    AddRef();
  }

  template <typename U>
  Ptr(Ptr<U> &&other)  // NOLINT
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

  Ptr &operator=(Ptr &&other) noexcept
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
    Release();
    ptr_ = nullptr;
  }

  explicit operator bool() const noexcept
  {
    return ptr_ != nullptr;
  }

  T *operator->() const noexcept
  {
    return ptr_;
  }

  T &operator*() const noexcept
  {
    return *ptr_;
  }

  std::size_t RefCount() const noexcept
  {
    return ptr_->ref_count_;
  }

private:
  T *ptr_ = nullptr;

  void AddRef()
  {
    if (ptr_)
    {
      ++(ptr_->ref_count_);
    }
  }

  void Release() noexcept
  {
    if (ptr_)
    {
      if (--(ptr_->ref_count_) == 0)
      {
        delete ptr_;
      }
    }
  }

  template <typename U>
  friend class Ptr;

  template <typename L, typename R>
  friend bool operator==(Ptr<L> const &lhs, Ptr<R> const &rhs) noexcept;  // NOLINT

  template <typename L>
  friend bool operator==(Ptr<L> const &lhs, std::nullptr_t /* rhs */) noexcept;  // NOLINT

  template <typename R>
  friend bool operator==(std::nullptr_t /* lhs */, Ptr<R> const &rhs) noexcept;  // NOLINT

  template <typename L, typename R>
  friend bool operator!=(Ptr<L> const &lhs, Ptr<R> const &rhs) noexcept;  // NOLINT

  template <typename L>
  friend bool operator!=(Ptr<L> const &lhs, std::nullptr_t /* rhs */) noexcept;  // NOLINT

  template <typename R>
  friend bool operator!=(std::nullptr_t /* lhs */, Ptr<R> const &rhs) noexcept;  // NOLINT
};

template <typename L, typename R>
bool operator==(Ptr<L> const &lhs, Ptr<R> const &rhs) noexcept
{
  return (lhs.ptr_ == static_cast<L *>(rhs.ptr_));
}

template <typename L>
bool operator==(Ptr<L> const &lhs, std::nullptr_t /* rhs */) noexcept
{
  return (lhs.ptr_ == nullptr);
}

template <typename R>
bool operator==(std::nullptr_t /* lhs */, Ptr<R> const &rhs) noexcept
{
  return (nullptr == rhs.ptr_);
}

template <typename L, typename R>
bool operator!=(Ptr<L> const &lhs, Ptr<R> const &rhs) noexcept
{
  return (lhs.ptr_ != static_cast<L *>(rhs.ptr_));
}

template <typename L>
bool operator!=(Ptr<L> const &lhs, std::nullptr_t /* rhs */) noexcept
{
  return (lhs.ptr_ != nullptr);
}

template <typename R>
bool operator!=(std::nullptr_t /* lhs */, Ptr<R> const &rhs) noexcept
{
  return (nullptr != rhs.ptr_);
}

class Unknown;

template <template <typename T, typename... Args> class Functor, typename... Args>
auto TypeIdAsCanonicalType(TypeId const type_id, Args &&... args)
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
