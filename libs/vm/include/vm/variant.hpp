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

#include "meta/pack.hpp"
#include "meta/switch.hpp"
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
      bool const is_object       = IsPrimitive() == false;
      bool const other_is_object = other.IsPrimitive() == false;
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
      bool const is_object       = IsPrimitive() == false;
      bool const other_is_object = other.IsPrimitive() == false;
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
    if (IsPrimitive() == false)
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
    if (IsPrimitive() == false)
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

template<TypeId id> struct IdToTypeMap;

template<TypeId id> using IdToTypeMapT = typename IdToTypeMap<id>::type

template<> struct IdToTypeMap<TypeIds::Null>: type_util::Type<std::nullptr_t> {};

template<> struct IdToTypeMap<TypeIds::Void>: type_util::Type<void> {};

template<> struct IdToTypeMap<TypeIds::Bool>: type_util::Type<bool> {};

template<> struct IdToTypeMap<TypeIds::Int8>: type_util::Type<int8_t> {};

template<> struct IdToTypeMap<TypeIds::UInt8>: type_util::Type<uint8_t> {};

template<> struct IdToTypeMap<TypeIds::Int16>: type_util::Type<int16_t> {};

template<> struct IdToTypeMap<TypeIds::UInt16>: type_util::Type<uint16_t> {};

template<> struct IdToTypeMap<TypeIds::Int32>: type_util::Type<int32_t> {};

template<> struct IdToTypeMap<TypeIds::UInt32>: type_util::Type<uint32_t> {};

template<> struct IdToTypeMap<TypeIds::Int64>: type_util::Type<int64_t> {};

template<> struct IdToTypeMap<TypeIds::UInt64>: type_util::Type<uint64_t> {};

template<> struct IdToTypeMap<TypeIds::Float32>: type_util::Type<float> {};

template<> struct IdToTypeMap<TypeIds::Float64>: type_util::Type<double> {};

template<> struct IdToTypeMap<TypeIds::Fixed32>: type_util::Type<fixed_point::fp32_t> {};

template<> struct IdToTypeMap<TypeIds::Fixed64>: type_util::Type<fixed_point::fp64_t> {};

template<> struct IdToTypeMap<TypeIds::String>: type_util::Type<Ptr<String>> {};

template<> struct IdToTypeMap<TypeIds::Address>: type_util::Type<Ptr<Address>> {};

template<class T> struct TypeToIdMap;

template<class T> static constexpr auto TypeToIdMapV = TypeToIdMap<T>::value;

template<> struct TypeToIdMap<std::nullptr_t>: std::integral_constant<TypeId, TypeIds::Null> {};

template<> struct TypeToIdMap<void>: std::integral_constant<TypeId, TypeIds::Void> {};

template<> struct TypeToIdMap<bool>: std::integral_constant<TypeId, TypeIds::Bool> {};

template<> struct TypeToIdMap<int8_t>: std::integral_constant<TypeId, TypeIds::Int8> {};

template<> struct TypeToIdMap<uint8_t>: std::integral_constant<TypeId, TypeIds::UInt8> {};

template<> struct TypeToIdMap<int16_t>: std::integral_constant<TypeId, TypeIds::Int16> {};

template<> struct TypeToIdMap<uint16_t>: std::integral_constant<TypeId, TypeIds::UInt16> {};

template<> struct TypeToIdMap<int32_t>: std::integral_constant<TypeId, TypeIds::Int32> {};

template<> struct TypeToIdMap<uint32_t>: std::integral_constant<TypeId, TypeIds::UInt32> {};

template<> struct TypeToIdMap<int64_t>: std::integral_constant<TypeId, TypeIds::Int64> {};

template<> struct TypeToIdMap<uint64_t>: std::integral_constant<TypeId, TypeIds::UInt64> {};

template<> struct TypeToIdMap<float>: std::integral_constant<TypeId, TypeIds::Float32> {};

template<> struct TypeToIdMap<double>: std::integral_constant<TypeId, TypeIds::Float64> {};

template<> struct TypeToIdMap<fixed_point::fp32_t>: std::integral_constant<TypeId, TypeIds::Fixed32> {};

template<> struct TypeToIdMap<fixed_point::fp64_t>: std::integral_constant<TypeId, TypeIds::Fixed64> {};

template<> struct TypeToIdMap<Ptr<String>>: std::integral_constant<TypeId, TypeIds::String> {};

template<> struct TypeToIdMap<Ptr<Address>>: std::integral_constant<TypeId, TypeIds::Address> {};

template<class T, bool = IsPrimitive<T>::value> class VariantView
  : public std::integral_constant<TypeId, TypeToIdMapV<T>>
{
	Variant *v_;

	constexpr VariantView(Variant &v) noexcept: v_(&v) {}
public:
	using type = T;

	constexpr auto Get() const noexcept { return v_->Get<type>(); }

	constexpr void Set(type val) noexcept { return v_->Set(val); }

	template<class F, class... Args> static constexpr Invoke(F &&f, Args &&...args) {
		return type_util::Invoke(std::forward<F>(f), VariantView(std::forward<Args>(args))...);
	}
};

template<class T> constexpr void VariantView<T, false>::Set(type val) noexcept { v_->Set(std::move(val)); }

template<> class VariantView<Ptr<Object>, false>
{
	Variant *v_;

	constexpr VariantView(Variant &v) noexcept: v_(&v) {}
public:
	using type = Ptr<Object>;

	constexpr auto Get() const noexcept { return v_->Get<type>(); }

	constexpr void Set(type val) noexcept { return v_->Set(std::move<val>); }

	template<class F, class... Args> static constexpr Invoke(F &&f, Args &&...args) {
		return type_util::Invoke(std::forward<F>(f), VariantView(std::forward<Args>(args))...);
	}
};

template<TypeId id> using TypeIdCase = VariantView<IdToTypeMapT<id>>;

template<TypeId... ids> using TypeIdCases = type_util::LiftIntegerSequenceT<std::integer_sequence<TypeId, ids...>, TypeIdCase>;

using DefaultObjectCase = VariantView<Ptr<Object>>;

using UnsignedIntegerTypes = TypeIdCases<TypeIds::UInt8, TypeIds::UInt16, TypeIds::UInt32, TypeIds::UInt64>;
using SignedIntegerTypes = TypeIdCases<TypeIds::Int8, TypeIds::Int16, TypeIds::Int32, TypeIds::Int64>;
using FloatingPointTypes = TypeIdCases<TypeIds::Float32, TypeIds::Float64>;
using FixedPointTypes = TypeIdCases<TypeIds::Fixed32, TypeIds::Fixed64>;

using IntegralTypes = pack::ConcatT<UnsignedIntegerTypes, SignedIntegerTypes>;

using NumericTypes = pack::ConcatT<IntegralTypes, FloatingPointTypes, FixedPointTypes>;

using ScalarTypes = pack::ConsT<TypeIdCase<TypeIds::Bool>, NumericTypes>;

using BuiltinTypes = pack::ConcatT<ScalarTypes, TypeIdCases<TypeIds::String, TypeIds::Address>>;

using NonInstantiatableTypes = TypeIdCases<TypeIds::Null, TypeIds::Void>;

template<class... Cases, class F, class... Variants> constexpr auto ApplyFunctor(TypeId type_id, F &&f, Variants &&...variants)
{
	return type_util::Switch<Cases...>::Invoke(type_id, std::forward<F>(f), std::forward<Variants>(variants)...);
}

template<class F, class... Variants> constexpr auto ApplyIntegralFunctor(TypeId type_id, F &&f, Variants &&...variants) { return ApplyFunctor<IntegralTypes>(type_id, std::forward<F>(f), std::forward<Variants>(variants)...); }

template<class F, class... Variants> constexpr auto ApplyNumericFunctor(TypeId type_id, F &&f, Variants &&...variants) { return ApplyFunctor<NumericTypes>(type_id, std::forward<F>(f), std::forward<Variants>(variants)...); }

template<class F, class... Variants> constexpr auto ApplyScalarFunctor(TypeId type_id, F &&f, Variants &&...variants) { return ApplyFunctor<ScalarTypes>(type_id, std::forward<F>(f), std::forward<Variants>(variants)...); }

template<class F, class... Variants> constexpr auto ApplyBuiltinFunctor(TypeId type_id, F &&f, Variants &&...variants) { return ApplyFunctor<BuiltinTypes>(type_id, std::forward<F>(f), std::forward<Variants>(variants)...); }

}  // namespace vm
}  // namespace fetch
