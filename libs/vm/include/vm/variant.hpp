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

#include "meta/value_util.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/object.hpp"

namespace fetch {
namespace vm {

union Primitive
{
  int8_t   i8;
  uint8_t  ui8;
  int16_t  i16;
  uint16_t ui16;
  int32_t  i32;
  uint32_t ui32;
  int64_t  i64;
  uint64_t ui64;
  float    f32;
  double   f64;

  void Zero()
  {
    ui64 = 0;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, bool>::value, T> Get() const
  {
    return bool(ui8);
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, int8_t>::value, T> Get() const
  {
    return i8;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, uint8_t>::value, T> Get() const
  {
    return ui8;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, int16_t>::value, T> Get() const
  {
    return i16;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, uint16_t>::value, T> Get() const
  {
    return ui16;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, int32_t>::value, T> Get() const
  {
    return i32;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, uint32_t>::value, T> Get() const
  {
    return ui32;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, int64_t>::value, T> Get() const
  {
    return i64;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, uint64_t>::value, T> Get() const
  {
    return ui64;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, float>::value, T> Get() const
  {
    return f32;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, double>::value, T> Get() const
  {
    return f64;
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, fixed_point::fp32_t>::value, T> Get() const
  {
    return fixed_point::fp32_t::FromBase(i32);
  }

  template <typename T>
  typename std::enable_if_t<std::is_same<T, fixed_point::fp64_t>::value, T> Get() const
  {
    return fixed_point::fp64_t::FromBase(i64);
  }

  void Set(bool value)
  {
    ui8 = uint8_t(value);
  }

  void Set(int8_t value)
  {
    i8 = value;
  }

  void Set(uint8_t value)
  {
    ui8 = value;
  }

  void Set(int16_t value)
  {
    i16 = value;
  }

  void Set(uint16_t value)
  {
    ui16 = value;
  }

  void Set(int32_t value)
  {
    i32 = value;
  }

  void Set(uint32_t value)
  {
    ui32 = value;
  }

  void Set(int64_t value)
  {
    i64 = value;
  }

  void Set(uint64_t value)
  {
    ui64 = value;
  }

  void Set(float value)
  {
    f32 = value;
  }

  void Set(double value)
  {
    f64 = value;
  }

  void Set(fixed_point::fp32_t value)
  {
    i32 = value.Data();
  }

  void Set(fixed_point::fp64_t value)
  {
    i64 = value.Data();
  }
};

struct Variant
{
  union
  {
    Primitive   primitive;
    Ptr<Object> object;
  };
  TypeId type_id;

  Variant()
  {
    Construct();
  }

  Variant(Variant const &other)
  {
    Construct(other);
  }

  Variant(Variant &&other)
  {
    Construct(std::move(other));
  }

  template <typename T, typename std::enable_if_t<IsPrimitive<T>::value> * = nullptr>
  Variant(T other, TypeId other_type_id)
  {
    Construct(other, other_type_id);
  }

  template <typename T, typename std::enable_if_t<IsPtr<T>::value> * = nullptr>
  Variant(T const &other, TypeId other_type_id)
  {
    Construct(other, other_type_id);
  }

  template <typename T, typename std::enable_if_t<IsPtr<T>::value> * = nullptr>
  Variant(T &&other, TypeId other_type_id)
  {
    Construct(std::forward<T>(other), other_type_id);
  }

  Variant(Primitive other, TypeId other_type_id)
  {
    Construct(other, other_type_id);
  }

  void Construct()
  {
    type_id = TypeIds::Unknown;
  }

  void Construct(Variant const &other)
  {
    type_id = other.type_id;
    if (IsPrimitive())
    {
      primitive = other.primitive;
    }
    else
    {
      new (&object) Ptr<Object>(other.object);
    }
  }

  void Construct(Variant &&other)
  {
    type_id = other.type_id;
    if (IsPrimitive())
    {
      primitive = other.primitive;
    }
    else
    {
      new (&object) Ptr<Object>(std::move(other.object));
    }
    other.type_id = TypeIds::Unknown;
  }

  template <typename T, typename std::enable_if_t<IsPrimitive<T>::value> * = nullptr>
  void Construct(T other, TypeId other_type_id)
  {
    primitive.Set(other);
    type_id = other_type_id;
  }

  template <typename T, typename std::enable_if_t<IsPtr<T>::value> * = nullptr>
  void Construct(T const &other, TypeId other_type_id)
  {
    new (&object) Ptr<Object>(other);
    type_id = other_type_id;
  }

  template <typename T, typename std::enable_if_t<IsPtr<T>::value> * = nullptr>
  void Construct(T &&other, TypeId other_type_id)
  {
    new (&object) Ptr<Object>(std::forward<T>(other));
    type_id = other_type_id;
  }

  void Construct(Primitive other, TypeId other_type_id)
  {
    primitive = other;
    type_id   = other_type_id;
  }

  Variant &operator=(Variant const &other)
  {
    if (this != &other)
    {
      bool const is_object       = !IsPrimitive();
      bool const other_is_object = !other.IsPrimitive();
      type_id                    = other.type_id;
      if (is_object)
      {
        if (other_is_object)
        {
          // Copy object to current object
          object = other.object;
        }
        else
        {
          // Copy primitive to current object
          object.Reset();
          primitive = other.primitive;
        }
      }
      else
      {
        if (other_is_object)
        {
          // Copy object to current primitive
          new (&object) Ptr<Object>(other.object);
        }
        else
        {
          // Copy primitive to current primitive
          primitive = other.primitive;
        }
      }
    }
    return *this;
  }

  Variant &operator=(Variant &&other)
  {
    if (this != &other)
    {
      bool const is_object       = !IsPrimitive();
      bool const other_is_object = !other.IsPrimitive();
      type_id                    = other.type_id;
      other.type_id              = TypeIds::Unknown;
      if (is_object)
      {
        if (other_is_object)
        {
          // Move object to current object
          object = std::move(other.object);
        }
        else
        {
          // Move primitive to current object
          object.Reset();
          primitive = other.primitive;
        }
      }
      else
      {
        if (other_is_object)
        {
          // Move object to current primitive
          new (&object) Ptr<Object>(std::move(other.object));
        }
        else
        {
          // Move primitive to current primitive
          primitive = other.primitive;
        }
      }
    }
    return *this;
  }

  template <typename T, typename std::enable_if_t<IsPrimitive<T>::value> * = nullptr>
  void Assign(T other, TypeId other_type_id)
  {
    if (!IsPrimitive())
    {
      object.Reset();
    }
    primitive.Set(other);
    type_id = other_type_id;
  }

  template <typename T, typename std::enable_if_t<IsPtr<T>::value> * = nullptr>
  void Assign(T const &other, TypeId other_type_id)
  {
    if (IsPrimitive())
    {
      Construct(other, other_type_id);
    }
    else
    {
      object  = other;
      type_id = other_type_id;
    }
  }

  template <typename T, typename std::enable_if_t<IsPtr<T>::value> * = nullptr>
  void Assign(T &&other, TypeId other_type_id)
  {
    if (IsPrimitive())
    {
      Construct(std::forward<T>(other), other_type_id);
    }
    else
    {
      object  = std::forward<T>(other);
      type_id = other_type_id;
    }
  }

  template <typename T, typename std::enable_if_t<IsVariant<T>::value> * = nullptr>
  void Assign(T const &other, TypeId /* other_type_id */)
  {
    operator=(other);
  }

  template <typename T, typename std::enable_if_t<IsVariant<T>::value> * = nullptr>
  void Assign(T &&other, TypeId /* other_type_id */)
  {
    operator=(std::forward<T>(other));
  }

  template <typename T>
  typename std::enable_if_t<IsPrimitive<T>::value, T> Get() const
  {
    return primitive.Get<T>();
  }

  template <typename T>
  typename std::enable_if_t<IsPtr<T>::value, T> Get() const
  {
    return object;
  }

  template <typename T>
  typename std::enable_if_t<IsVariant<T>::value, T> Get() const
  {
    T variant;
    variant.type_id = type_id;
    if (IsPrimitive())
    {
      variant.primitive = primitive;
    }
    else
    {
      new (&variant.object) Ptr<Object>(object);
    }
    return variant;
  }

  template <typename T>
  typename std::enable_if_t<IsPrimitive<T>::value, T> Move()
  {
    type_id = TypeIds::Unknown;
    return primitive.Get<T>();
  }

  template <typename T>
  typename std::enable_if_t<IsPtr<T>::value, T> Move()
  {
    type_id = TypeIds::Unknown;
    return std::move(object);
  }

  template <typename T>
  typename std::enable_if_t<IsVariant<T>::value, T> Move()
  {
    T variant;
    variant.type_id = type_id;
    if (IsPrimitive())
    {
      variant.primitive = primitive;
    }
    else
    {
      new (&variant.object) Ptr<Object>(std::move(object));
    }
    type_id = TypeIds::Unknown;
    return variant;
  }

  ~Variant()
  {
    Reset();
  }

  bool IsPrimitive() const
  {
    return type_id <= TypeIds::PrimitiveMaxId;
  }

  void Reset()
  {
    if (!IsPrimitive())
    {
      object.Reset();
    }
    type_id = TypeIds::Unknown;
  }
};

struct TemplateParameter1 : public Variant
{
  using Variant::Variant;
};

struct TemplateParameter2 : public Variant
{
  using Variant::Variant;
};

struct Any : public Variant
{
  using Variant::Variant;
};

struct AnyPrimitive : public Variant
{
  using Variant::Variant;
};

struct AnyInteger : public Variant
{
  using Variant::Variant;
};

struct AnyFloatingPoint : public Variant
{
  using Variant::Variant;
};

namespace detail_ {

// Native converts TypeId to C++ type
template <TypeId type_id>
struct Native;
template <TypeId type_id>
using NativeT = typename Native<type_id>::type;

template <>
struct Native<TypeIds::Null> : type_util::TypeConstant<std::nullptr_t>
{
};
template <>
struct Native<TypeIds::Void> : type_util::TypeConstant<void>
{
};
template <>
struct Native<TypeIds::Bool> : type_util::TypeConstant<bool>
{
};
template <>
struct Native<TypeIds::Int8> : type_util::TypeConstant<std::int8_t>
{
};
template <>
struct Native<TypeIds::UInt8> : type_util::TypeConstant<std::uint8_t>
{
};
template <>
struct Native<TypeIds::Int16> : type_util::TypeConstant<std::int16_t>
{
};
template <>
struct Native<TypeIds::UInt16> : type_util::TypeConstant<std::uint16_t>
{
};
template <>
struct Native<TypeIds::Int32> : type_util::TypeConstant<std::int32_t>
{
};
template <>
struct Native<TypeIds::UInt32> : type_util::TypeConstant<std::uint32_t>
{
};
template <>
struct Native<TypeIds::Int64> : type_util::TypeConstant<std::int64_t>
{
};
template <>
struct Native<TypeIds::UInt64> : type_util::TypeConstant<std::uint64_t>
{
};
template <>
struct Native<TypeIds::Float32> : type_util::TypeConstant<float>
{
};
template <>
struct Native<TypeIds::Float64> : type_util::TypeConstant<double>
{
};
template <>
struct Native<TypeIds::Fixed32> : type_util::TypeConstant<fixed_point::fp32_t>
{
};
template <>
struct Native<TypeIds::Fixed64> : type_util::TypeConstant<fixed_point::fp64_t>
{
};
template <>
struct Native<TypeIds::String> : type_util::TypeConstant<std::string>
{
};
template <>
struct Native<TypeIds::Address> : type_util::TypeConstant<void *>
{
};
// As a special case, to be used for objects.
//
// The logic here is approximately the same that the Committee had
// when they finally allowed void specializations of standard callable objects in STL
// (e.g. std::less<void>): as it is impractical to compare two values of type void,
// which both cannot even exist, we can give this overload a slightly special meaning.
// Same goes here for UnknownType.
template <>
struct Native<TypeIds::Unknown> : type_util::TypeConstant<Ptr<Object>>
{
};

// TypeIdFor provides an inverse mapping, defines the best TypeId for a given C++ type
template <typename Native>
struct TypeIdFor : TypeIdFor<std::decay_t<Native>>
{
};
template <typename Native>
static constexpr auto TypeIdForV = TypeIdFor<Native>::value;

template <TypeId id>
using TypeIdConstant = std::integral_constant<TypeId, id>;

template <>
struct TypeIdFor<std::nullptr_t> : TypeIdConstant<TypeIds::Null>
{
};
template <>
struct TypeIdFor<void> : TypeIdConstant<TypeIds::Void>
{
};
template <>
struct TypeIdFor<bool> : TypeIdConstant<TypeIds::Bool>
{
};
template <>
struct TypeIdFor<std::int8_t> : TypeIdConstant<TypeIds::Int8>
{
};
template <>
struct TypeIdFor<std::uint8_t> : TypeIdConstant<TypeIds::UInt8>
{
};
template <>
struct TypeIdFor<std::int16_t> : TypeIdConstant<TypeIds::Int16>
{
};
template <>
struct TypeIdFor<std::uint16_t> : TypeIdConstant<TypeIds::UInt16>
{
};
template <>
struct TypeIdFor<std::int32_t> : TypeIdConstant<TypeIds::Int32>
{
};
template <>
struct TypeIdFor<std::uint32_t> : TypeIdConstant<TypeIds::UInt32>
{
};
template <>
struct TypeIdFor<std::int64_t> : TypeIdConstant<TypeIds::Int64>
{
};
template <>
struct TypeIdFor<std::uint64_t> : TypeIdConstant<TypeIds::UInt64>
{
};
template <>
struct TypeIdFor<float> : TypeIdConstant<TypeIds::Float32>
{
};
template <>
struct TypeIdFor<double> : TypeIdConstant<TypeIds::Float64>
{
};
template <>
struct TypeIdFor<fixed_point::fp32_t> : TypeIdConstant<TypeIds::Fixed32>
{
};
template <>
struct TypeIdFor<fixed_point::fp64_t> : TypeIdConstant<TypeIds::Fixed64>
{
};
template <>
struct TypeIdFor<std::string> : TypeIdConstant<TypeIds::String>
{
};
template <>
struct TypeIdFor<void *> : TypeIdConstant<TypeIds::Address>
{
};
// For consistency with the above definition
template <>
struct TypeIdFor<Ptr<Object>> : TypeIdConstant<TypeIds::Unknown>
{
};

// Storage type actually used in the Variant. This differs from Native in bool.
template <TypeId type_id>
struct Storage : Native<type_id>
{
};
template <TypeId type_id>
using StorageT = typename Storage<type_id>::type;

template <>
struct Storage<TypeIds::Bool> : Native<TypeIds::UInt8>
{
};

template <typename NativeType>
using StorageType = StorageT<TypeIdForV<NativeType>>;

template <class ParticularTypeId, class Variant>
class TypedView
{
  Variant &&observed_;

public:
  static constexpr TypeId type_id = ParticularTypeId::value;
  using type                      = NativeT<type_id>;

  TypedView(Variant &&observed)
    : observed_(std::forward<Variant>(observed))
  {}

  template <typename T>
  operator T() const
  {
    return static_cast<T>(observed_.template Get<type>());
  }

  operator fixed_point::fp32_t() const
  {
    return fixed_point::fp32_t::FromBase(static_cast<int>(observed_.template Get<type>()));
  }

  operator fixed_point::fp64_t() const
  {
    return fixed_point::fp64_t::FromBase(static_cast<int64_t>(observed_.template Get<type>()));
  }

  template <typename Value>
  TypedView &operator=(Value value)
  {
    observed_.template Assign(value, type_id);
    return *this;
  }

  operator Variant &&() const
  {
    return std::forward<Variant>(observed_);
  }

  type Value() const
  {
    return observed_.template Get<type>();
  }
};

template <class Stream, class ParticularTypeId, class Variant>
inline void Serialize(Stream &&s, TypedView<ParticularTypeId, Variant> const &v)
{
  s << static_cast<typename std::decay_t<decltype(v)>::type>(v);
}

template <class Stream, class ParticularTypeId, class Variant>
inline void Deserialize(Stream &&s, TypedView<ParticularTypeId, Variant> &v)
{
  typename std::decay_t<decltype(v)>::type value_received;
  std::forward<Stream>(s) >> value_received;
  v = value_received;
}

template <class Accum, class ParticularTypeId>
struct InvokeOnId
{
  template <class F, class... Variants>
  static decltype(auto) Call(TypeId type_id, F &&f, Variants &&... args) noexcept(
      noexcept(type_id == ParticularTypeId::value
                   ? value_util::Invoke(std::forward<F>(f), TypedView<ParticularTypeId, Variants>(
                                                                std::forward<Variants>(args))...)
                   : Accum::Call(type_id, std::forward<F>(f), std::forward<Variants>(args)...)))
  {
    return type_id == ParticularTypeId::value
               ? value_util::Invoke(
                     std::forward<F>(f),
                     (TypedView<ParticularTypeId, Variants>(std::forward<Variants>(args)))...)
               : Accum::Call(type_id, std::forward<F>(f), std::forward<Variants>(args)...);
  }
  template <class F>
  static decltype(auto) Call(TypeId type_id, F &&f) noexcept(
      noexcept(type_id == ParticularTypeId::value
                   ? value_util::Invoke(std::forward<F>(f), ParticularTypeId{})
                   : Accum::Call(type_id, std::forward<F>(f))))
  {
    return type_id == ParticularTypeId::value
               ? value_util::Invoke(std::forward<F>(f), ParticularTypeId{})
               : Accum::Call(type_id, std::forward<F>(f));
  }
};

template <class Accum>
struct InvokeOnId<Accum, TypeIdConstant<TypeIds::Unknown>>
{
  using ParticularTypeId = TypeIdConstant<TypeIds::Unknown>;
  template <class F, class... Variants>
  static decltype(auto) Call(TypeId type_id, F &&f, Variants &&... args) noexcept(
      noexcept(type_id > TypeIds::PrimitiveMaxId
                   ? value_util::Invoke(std::forward<F>(f), TypedView<ParticularTypeId, Variants>(
                                                                std::forward<Variants>(args))...)
                   : Accum::Call(type_id, std::forward<F>(f), std::forward<Variants>(args)...)))
  {
    return type_id > TypeIds::PrimitiveMaxId
               ? value_util::Invoke(
                     std::forward<F>(f),
                     (TypedView<ParticularTypeId, Variants>(std::forward<Variants>(args)))...)
               : Accum::Call(type_id, std::forward<F>(f), std::forward<Variants>(args)...);
  }
  template <class F>
  static decltype(auto) Call(TypeId type_id, F &&f) noexcept(noexcept(
      type_id > TypeIds::PrimitiveMaxId ? value_util::Invoke(std::forward<F>(f), ParticularTypeId{})
                                        : Accum::Call(type_id, std::forward<F>(f))))
  {
    return type_id > TypeIds::PrimitiveMaxId
               ? value_util::Invoke(std::forward<F>(f), ParticularTypeId{})
               : Accum::Call(type_id, std::forward<F>(f));
  }
};

template <class FirstTypeId>
class Zero
{
  template <class T>
  using ArgType = TypedView<FirstTypeId, T>;

public:
  template <class F, class... Ts>
  static constexpr decltype(auto) Call(TypeId, F &&, Ts &&...) noexcept(
      noexcept(type_util::ReturnZero<meta::InvokeResultT<F, ArgType<Ts>...>>::Call()))
  {
    using RV = meta::InvokeResultT<F, ArgType<Ts>...>;
    return type_util::ReturnZero<RV>::Call();
  }
  template <class F, class... Ts>
  static constexpr decltype(auto) Call(TypeId, F &&) noexcept(
      noexcept(type_util::ReturnZero<meta::InvokeResultT<F, FirstTypeId>>::Call()))
  {
    using RV = meta::InvokeResultT<F, FirstTypeId>;
    return type_util::ReturnZero<RV>::Call();
  }
};

}  // namespace detail_

template <TypeId... accepted_ids, class F, class... Variants>
constexpr auto ApplyFunctor(TypeId selector_id, F &&f, Variants &&... args) noexcept(noexcept(
    type_util::AccumulateT<
        detail_::InvokeOnId,
        detail_::Zero<type_util::HeadT<std::integral_constant<TypeId, accepted_ids>...>>,
        std::integral_constant<TypeId, accepted_ids>...>::Call(selector_id, std::forward<F>(f),
                                                               std::forward<Variants>(args)...)))
{
  // assert that types are known and all the same
  assert(value_util::Accumulate(
             [](TypeId accum, Variant const &arg) {
               if (arg.type_id == accum)
               {
                 return accum;
               }
               return TypeIds::Unknown;
             },
             selector_id, args...) != TypeIds::Unknown);

  // Deduce returned type based on the first given accepted_id,
  // as returned type should be always the same no matter what accepted_ids are.

  using FirstAcceptedTypeId = type_util::HeadT<std::integral_constant<TypeId, accepted_ids>...>;

  using Executor = type_util::AccumulateT<detail_::InvokeOnId, detail_::Zero<FirstAcceptedTypeId>,
                                          std::integral_constant<TypeId, accepted_ids>...>;

  return Executor::Call(selector_id, std::forward<F>(f), std::forward<Variants>(args)...);
}

template <class F, class... Variants>
constexpr auto ApplyIntegralFunctor(TypeId type_id, F &&f, Variants &&... args) noexcept(
    noexcept(ApplyFunctor<TypeIds::UInt8, TypeIds::Int8, TypeIds::Int16, TypeIds::UInt16,
                          TypeIds::Int32, TypeIds::UInt32, TypeIds::Int64, TypeIds::UInt64>(
        type_id, std::forward<F>(f), std::forward<Variants>(args)...)))
{
  return ApplyFunctor<TypeIds::UInt8, TypeIds::Int8, TypeIds::Int16, TypeIds::UInt16,
                      TypeIds::Int32, TypeIds::UInt32, TypeIds::Int64, TypeIds::UInt64>(
      type_id, std::forward<F>(f), std::forward<Variants>(args)...);
}

template <class F, class... Variants>
constexpr auto ApplyNumericFunctor(TypeId type_id, F &&f, Variants &&... args) noexcept(
    noexcept(ApplyFunctor<TypeIds::UInt8, TypeIds::Int8, TypeIds::Int16, TypeIds::UInt16,
                          TypeIds::Int32, TypeIds::UInt32, TypeIds::Int64, TypeIds::UInt64,
                          TypeIds::Fixed32, TypeIds::Fixed64, TypeIds::Float32, TypeIds::Float64>(
        type_id, std::forward<F>(f), std::forward<Variants>(args)...)))
{
  return ApplyFunctor<TypeIds::UInt8, TypeIds::Int8, TypeIds::Int16, TypeIds::UInt16,
                      TypeIds::Int32, TypeIds::UInt32, TypeIds::Int64, TypeIds::UInt64,
                      TypeIds::Fixed32, TypeIds::Fixed64, TypeIds::Float32, TypeIds::Float64>(
      type_id, std::forward<F>(f), std::forward<Variants>(args)...);
}

template <class F, class... Variants>
constexpr auto ApplyScalarFunctor(TypeId type_id, F &&f, Variants &&... args) noexcept(noexcept(
    ApplyFunctor<TypeIds::Bool, TypeIds::UInt8, TypeIds::Int8, TypeIds::Int16, TypeIds::UInt16,
                 TypeIds::Int32, TypeIds::UInt32, TypeIds::Int64, TypeIds::UInt64, TypeIds::Fixed32,
                 TypeIds::Fixed64, TypeIds::Float32, TypeIds::Float64>(
        type_id, std::forward<F>(f), std::forward<Variants>(args)...)))
{
  return ApplyFunctor<TypeIds::Bool, TypeIds::UInt8, TypeIds::Int8, TypeIds::Int16, TypeIds::UInt16,
                      TypeIds::Int32, TypeIds::UInt32, TypeIds::Int64, TypeIds::UInt64,
                      TypeIds::Fixed32, TypeIds::Fixed64, TypeIds::Float32, TypeIds::Float64>(
      type_id, std::forward<F>(f), std::forward<Variants>(args)...);
}

template <class F, class... Variants>
constexpr auto ApplyFullFunctor(TypeId type_id, F &&f, Variants &&... args) noexcept(noexcept(
    ApplyFunctor<TypeIds::Bool, TypeIds::UInt8, TypeIds::Int8, TypeIds::Int16, TypeIds::UInt16,
                 TypeIds::Int32, TypeIds::UInt32, TypeIds::Int64, TypeIds::UInt64, TypeIds::Fixed32,
                 TypeIds::Fixed64, TypeIds::Float32, TypeIds::Float64, TypeIds::Unknown>(
        type_id, std::forward<F>(f), std::forward<Variants>(args)...)))
{
  return ApplyFunctor<TypeIds::Bool, TypeIds::UInt8, TypeIds::Int8, TypeIds::Int16, TypeIds::UInt16,
                      TypeIds::Int32, TypeIds::UInt32, TypeIds::Int64, TypeIds::UInt64,
                      TypeIds::Fixed32, TypeIds::Fixed64, TypeIds::Float32, TypeIds::Float64,
                      TypeIds::Unknown>(type_id, std::forward<F>(f),
                                        std::forward<Variants>(args)...);
}

template <typename Integral = std::int64_t>
inline Integral GetIntegral(Variant const &v)
{
  return ApplyIntegralFunctor(v.type_id, [](auto &&v) { return static_cast<Integral>(v); }, v);
}

template <typename Natural = std::uint64_t>
inline Natural GetNatural(Variant const &v)
{
  return ApplyIntegralFunctor(
      v.type_id, [](auto &&v) { return v.Value() > 0 ? static_cast<Natural>(v) : Natural{}; }, v);
}

}  // namespace vm
}  // namespace fetch
