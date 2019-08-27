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
#include "meta/slots.hpp"
#include "meta/switch.hpp"
#include "meta/type_util.hpp"
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
    return {std::move(object)};
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

/**
 * Provides some useful information for a given type id:
 *     IdToType<type_id>::value        is the type_id itself;
 *     IdToType<type_id>::type         is the C++ type this id corresponds to;
 *     IdToType<type_id>::storage_type is the type used to store values of this type id inside
 *         Variant and in containers.
 *
 * In addition, some instantiations define static method
 *     storage_type &Reference(Variant &v)
 * that returns a reference to the value inside variant, and a constref-version of the same.
 *
 * Warning: the lifetime of a referred field should have already begun.
 *
 * @tparam id
 */
template <TypeId id>
struct IdToType;

template <TypeId id, typename T, typename U = T>
struct VariantValue : std::integral_constant<TypeId, id>
{
  using std::integral_constant<TypeId, id>::value;
  using type         = T;
  using storage_type = U;
};

template <>
struct IdToType<TypeIds::Unknown> : VariantValue<TypeIds::Unknown, Unknown>
{
};

template <>
struct IdToType<TypeIds::Null> : VariantValue<TypeIds::Null, std::nullptr_t>
{
};

template <>
struct IdToType<TypeIds::Void> : VariantValue<TypeIds::Void, void>
{
};

template <>
struct IdToType<TypeIds::Bool> : VariantValue<TypeIds::Bool, bool, uint8_t>
{
  static constexpr auto &Reference(Variant &v) noexcept
  {
    return v.primitive.ui8;
  }
  static constexpr auto &Reference(Variant const &v) noexcept
  {
    return v.primitive.ui8;
  }
};

template <>
struct IdToType<TypeIds::Int8> : VariantValue<TypeIds::Int8, int8_t>
{
  static constexpr auto &Reference(Variant &v) noexcept
  {
    return v.primitive.i8;
  }
  static constexpr auto &Reference(Variant const &v) noexcept
  {
    return v.primitive.i8;
  }
};

template <>
struct IdToType<TypeIds::UInt8> : VariantValue<TypeIds::UInt8, uint8_t>
{
  static constexpr auto &Reference(Variant &v) noexcept
  {
    return v.primitive.ui8;
  }
  static constexpr auto &Reference(Variant const &v) noexcept
  {
    return v.primitive.ui8;
  }
};

template <>
struct IdToType<TypeIds::Int16> : VariantValue<TypeIds::Int16, int16_t>
{
  static constexpr auto &Reference(Variant &v) noexcept
  {
    return v.primitive.i16;
  }
  static constexpr auto &Reference(Variant const &v) noexcept
  {
    return v.primitive.i16;
  }
};

template <>
struct IdToType<TypeIds::UInt16> : VariantValue<TypeIds::UInt16, uint16_t>
{
  static constexpr auto &Reference(Variant &v) noexcept
  {
    return v.primitive.ui16;
  }
  static constexpr auto &Reference(Variant const &v) noexcept
  {
    return v.primitive.ui16;
  }
};

template <>
struct IdToType<TypeIds::Int32> : VariantValue<TypeIds::Int32, int32_t>
{
  static constexpr auto &Reference(Variant &v) noexcept
  {
    return v.primitive.i32;
  }
  static constexpr auto &Reference(Variant const &v) noexcept
  {
    return v.primitive.i32;
  }
};

template <>
struct IdToType<TypeIds::UInt32> : VariantValue<TypeIds::UInt32, uint32_t>
{
  static constexpr auto &Reference(Variant &v) noexcept
  {
    return v.primitive.ui32;
  }
  static constexpr auto &Reference(Variant const &v) noexcept
  {
    return v.primitive.ui32;
  }
};

template <>
struct IdToType<TypeIds::Int64> : VariantValue<TypeIds::Int64, int64_t>
{
  static constexpr auto &Reference(Variant &v) noexcept
  {
    return v.primitive.i64;
  }
  static constexpr auto &Reference(Variant const &v) noexcept
  {
    return v.primitive.i64;
  }
};

template <>
struct IdToType<TypeIds::UInt64> : VariantValue<TypeIds::UInt64, uint64_t>
{
  static constexpr auto &Reference(Variant &v) noexcept
  {
    return v.primitive.ui64;
  }
  static constexpr auto &Reference(Variant const &v) noexcept
  {
    return v.primitive.ui64;
  }
};

template <>
struct IdToType<TypeIds::Float32> : VariantValue<TypeIds::Float32, float>
{
  static constexpr auto &Reference(Variant &v) noexcept
  {
    return v.primitive.f32;
  }
  static constexpr auto &Reference(Variant const &v) noexcept
  {
    return v.primitive.f32;
  }
};

template <>
struct IdToType<TypeIds::Float64> : VariantValue<TypeIds::Float64, double>
{
  static constexpr auto &Reference(Variant &v) noexcept
  {
    return v.primitive.f64;
  }
  static constexpr auto &Reference(Variant const &v) noexcept
  {
    return v.primitive.f64;
  }
};

template <>
struct IdToType<TypeIds::Fixed32> : VariantValue<TypeIds::Fixed32, fixed_point::fp32_t>
{
  static fixed_point::fp32_t &Reference(Variant &v) noexcept
  {
    return *reinterpret_cast<fixed_point::fp32_t *>(&v);
  }
  static fixed_point::fp32_t const &Reference(Variant const &v) noexcept
  {
    return *reinterpret_cast<fixed_point::fp32_t const *>(&v);
  }
};

template <>
struct IdToType<TypeIds::Fixed64> : VariantValue<TypeIds::Fixed64, fixed_point::fp64_t>
{
  static fixed_point::fp64_t &Reference(Variant &v) noexcept
  {
    return *reinterpret_cast<fixed_point::fp64_t *>(&v);
  }
  static fixed_point::fp64_t const &Reference(Variant const &v) noexcept
  {
    return *reinterpret_cast<fixed_point::fp64_t const *>(&v);
  }
};

template <>
struct IdToType<TypeIds::String> : VariantValue<TypeIds::String, Ptr<String>>
{
  static constexpr auto &Reference(Variant &v) noexcept
  {
    return v.object;
  }
  static constexpr auto const &Reference(Variant const &v) noexcept
  {
    return v.object;
  }
};

template <>
struct IdToType<TypeIds::Address> : VariantValue<TypeIds::Address, Ptr<Address>>
{
  static constexpr auto &Reference(Variant &v) noexcept
  {
    return v.object;
  }
  static constexpr auto const &Reference(Variant const &v) noexcept
  {
    return v.object;
  }
};

/**
 * Case alternative for type_util::Switch.
 * It stores a value of type Variant and provides access to exactly one of its fields,
 * according to IdToType, thus effectively mapping type ids known at run-time on compile-time
 * information such as types and variant struct members.
 *
 * @tparam IdToType an instance of IdToType template defined above
 */
template <class IdToType, bool = IsPrimitive<typename IdToType::type>::value>
class VariantView
  : public IdToType  // parent defines value, type, storage_type and, perhaps, Reference(Variant &)
{
  using Parent = IdToType;

protected:
  Variant *      v_  = nullptr;
  Variant const *cv_ = nullptr;

  constexpr VariantView(Variant &v) noexcept
    : v_(&v)
  {}
  constexpr VariantView(Variant const &cv) noexcept
    : cv_(&cv)
  {}
  using Parent::Reference;

public:
  using typename Parent::type;
  using typename Parent::storage_type;
  using Parent::value;

  /**
   * Read-only access to the value of a particular type inside the variant.
   *
   * @return cv's value of this type id, or that of v_, whichever set
   */
  constexpr auto Get() const noexcept
  {
    return cv_ ? cv_->Get<type>() : v_->Get<type>();
  }

  /**
   * Write-only access to the value of a particular type inside the variant.
   */
  constexpr void Set(type val) noexcept
  {
    v_->Assign(val, value);
  }

  /**
   * @return reference to the particular value kept inside the variant
   */
  constexpr auto &Ref() const noexcept
  {
    return Reference(*v_);
  }
  /**
   * @return const reference to the particular value kept inside the variant
   */
  constexpr auto const &CRef() const noexcept
  {
    return cv_ ? Reference(*cv_) : Reference(const_cast<Variant const &>(*v_));
  }

  constexpr Variant &Var() noexcept
  {
    return *v_;
  }
  constexpr Variant const &CVar() const noexcept
  {
    return cv_ ? *cv_ : *v_;
  }

  /**
   * Invoked by type_util::Switch when a selector type id matches IdToType::value.
   *
   * @param f templated function to be invoked of viewed arguments
   * @return
   * @param args... variants
   */
  template <class F, class... Args>
  static constexpr decltype(auto) Invoke(F &&f, Args &&... args)
  {
    return value_util::Invoke(std::forward<F>(f), VariantView(std::forward<Args>(args))...);
  }

  /**
   * A special overload to deliver particular type id to the callable when no variants processed.
   *
   * @param f templated unary function
   * @return
   */
  template <class F>
  static constexpr decltype(auto) Invoke(F &&f)
  {
    // IdToType provides members type, storage_type and value
    return value_util::Invoke(std::forward<F>(f), IdToType{});
  }
};

// Partial specialization of VariantView for object types.
// It inherits the otherwise non-existent VariantView<IdToType, true>,
// to reuse methods common to primitive and non-primitive types.
template <class IdToType>
class VariantView<IdToType, false> : protected VariantView<IdToType, true>
{
  using Parent = VariantView<IdToType, true>;
  using Parent::v_;
  using Parent::Parent;

public:
  using typename Parent::type;
  using typename Parent::storage_type;
  using Parent::value;

  using Parent::Get;
  using Parent::Ref;
  using Parent::CRef;
  using Parent::Var;
  using Parent::CVar;

  constexpr void Set(type val) noexcept
  {
    v_->Assign(std::move(val), value);
  }

  template <class F, class... Args>
  static constexpr decltype(auto) Invoke(F &&f, Args &&... args)
  {
    return value_util::Invoke(std::forward<F>(f), VariantView(std::forward<Args>(args))...);
  }

  template <class F>
  static constexpr decltype(auto) Invoke(F &&f)
  {
    return value_util::Invoke(std::forward<F>(f), IdToType{});
  }
};

template <>
class VariantView<IdToType<TypeIds::Null>, false>
  : public IdToType<TypeIds::Null>  // parent defines value, type and storage_type
{
  using Parent = IdToType;

protected:
  Variant *      v_  = nullptr;
  Variant const *cv_ = nullptr;

  constexpr VariantView(Variant &v) noexcept
    : v_(&v)
  {}
  constexpr VariantView(Variant const &cv) noexcept
    : cv_(&cv)
  {}

public:
  using typename Parent::type;
  using typename Parent::storage_type;
  using Parent::value;

  /**
   * Read-only access to the value of a particular type inside the variant.
   *
   * @return cv's value of this type id, or that of v_, whichever set
   */
  constexpr auto Get() const noexcept
  {
    return nullptr;
  }

  /**
   * Write-only access to the value of a particular type inside the variant.
   */
  constexpr void Set(std::nullptr_t) noexcept
  {}

  constexpr Variant &Var() noexcept
  {
    return *v_;
  }
  constexpr Variant const &CVar() const noexcept
  {
    return cv_ ? *cv_ : *v_;
  }

  /**
   * Invoked by type_util::Switch when a selector type id matches IdToType::value.
   *
   * @param f templated function to be invoked of viewed arguments
   * @param args... variants
   */
  template <class F, class... Args>
  static constexpr decltype(auto) Invoke(F &&f, Args &&... args)
  {
    return value_util::Invoke(std::forward<F>(f), VariantView(std::forward<Args>(args))...);
  }

  template <class F>
  static constexpr decltype(auto) Invoke(F &&f)
  {
    return value_util::Invoke(std::forward<F>(f), IdToType<TypeIds::Null>{});
  }
};

// Class DefaultVariantView is used for DefaultCase clauses of type_util::Switch.
class DefaultVariantView : public VariantValue<TypeIds::NumReserved, Ptr<Object>>
{
  using Parent = VariantValue<TypeIds::NumReserved, Ptr<Object>>;

  Variant *      v_  = nullptr;
  Variant const *cv_ = nullptr;

  constexpr DefaultVariantView(Variant &v) noexcept
    : v_(&v)
  {}
  constexpr DefaultVariantView(Variant const &v) noexcept
    : cv_(&v)
  {}

public:
  using Parent::type;
  using Parent::storage_type;
  using Parent::value;

  /**
   * Read-only access to the value of a particular type inside the variant.
   *
   * @return cv's value of this type id, or that of v_, whichever set
   */
  auto Get() const noexcept
  {
    return cv_ ? cv_->Get<type>() : v_->Get<type>();
  }

  /**
   * Write-only access to the value of a particular type inside the variant.
   */
  void Set(type val) noexcept
  {
    v_->Assign(std::move(val), value);
  }

  /**
   * @return reference to the particular value kept inside the variant
   */
  constexpr auto &Ref() const noexcept
  {
    return v_->object;
  }
  /**
   * @return const reference to the particular value kept inside the variant
   */
  constexpr auto const &CRef() const noexcept
  {
    return cv_ ? cv_->object : v_->object;
  }

  constexpr Variant &Var() noexcept
  {
    return *v_;
  }
  constexpr Variant const &CVar() const noexcept
  {
    return cv_ ? *cv_ : *v_;
  }

  /**
   * Invoked by type_util::Switch when a selector type id matches no other clause value.
   *
   * @param f templated function to be invoked of viewed arguments
   * @param args... variants
   */
  template <class F, class... Args>
  static constexpr decltype(auto) Invoke(F &&f, Args &&... args)
  {
    return value_util::Invoke(std::forward<F>(f), DefaultVariantView(std::forward<Args>(args))...);
  }

  template <class F>
  static constexpr decltype(auto) Invoke(F &&f)
  {
    return value_util::Invoke(std::forward<F>(f),
                              VariantValue<TypeIds::NumReserved, Ptr<Object>>{});
  }
};

template <TypeId id>
using IdView = VariantView<IdToType<id>>;

// Typeid sets
template <TypeId... ids>
using TypeIdSequence = pack::Pack<IdToType<ids>...>;

using UnsignedIntegerIds =
    TypeIdSequence<TypeIds::UInt8, TypeIds::UInt16, TypeIds::UInt32, TypeIds::UInt64>;

using SignedIntegerIds =
    TypeIdSequence<TypeIds::Int8, TypeIds::Int16, TypeIds::Int32, TypeIds::Int64>;

using IntegralTypeIds = pack::ConcatT<UnsignedIntegerIds, SignedIntegerIds>;

using FloatingPointIds = TypeIdSequence<TypeIds::Float32, TypeIds::Float64>;

using FixedPointIds = TypeIdSequence<TypeIds::Fixed32, TypeIds::Fixed64>;

using RealTypeIds = pack::ConcatT<FloatingPointIds, FixedPointIds>;

using NumericTypeIds = pack::ConcatT<IntegralTypeIds, RealTypeIds>;

using PrimitiveTypeIds = pack::ConsT<IdToType<TypeIds::Bool>, NumericTypeIds>;

using BuiltinTypeIds =
    pack::ConcatT<PrimitiveTypeIds, TypeIdSequence<TypeIds::String, TypeIds::Address>>;

using NonInstantiableTypeIds = TypeIdSequence<TypeIds::Null, TypeIds::Void>;

// Variant view sets, defined for those typeid sets
template <class TypeIdValue>
using AsVariantView = VariantView<TypeIdValue>;

template <class... TypeIdValues>
using TypeIdCases = pack::TransformT<AsVariantView, pack::ConcatT<TypeIdValues...>>;

template <TypeId... ids>
using TypeIdCaseSequence = TypeIdCases<TypeIdSequence<ids...>>;

using DefaultObjectCase = type_util::DefaultCase<DefaultVariantView>;

using NonInstantiableTypes = TypeIdCases<NonInstantiableTypeIds>;

using UnsignedIntegerTypes = TypeIdCases<UnsignedIntegerIds>;

using SignedIntegerTypes = TypeIdCases<SignedIntegerIds>;

using IntegralTypes = TypeIdCases<IntegralTypeIds>;

using FloatingPointTypes = TypeIdCases<FloatingPointIds>;

using FixedPointTypes = TypeIdCases<FixedPointIds>;

using RealTypes = TypeIdCases<RealTypeIds>;

using NumericTypes = TypeIdCases<NumericTypeIds>;

using PrimitiveTypes = TypeIdCases<PrimitiveTypeIds>;

using BuiltinTypes = TypeIdCases<BuiltinTypeIds>;

using NonInstantiableTypes = TypeIdCases<NonInstantiableTypeIds>;

/**
 * Invokes a functor on viewed variants, constructing views according to the type_id given.
 * f should return the same (up to implicit conversion) type for all possible view types.
 *
 * When invoked as ApplyFunctor<C1, C2...>(type_id, f, var1...), it compares type_id
 * against every Ci::value and, if Cn matches, passes control to f(Cn(var1), Cn(var2)...).
 *
 * If no vars given (i.e. ApplyFunctor<C1, C2...>(type_id, f), f is called with a special dummy
 * zero-size argument whose type provides static constexpr method value, equal to type_id,
 * and member types type and storage_type.
 *
 * When there's no match (i.e. there's no DefaultCase among Cases), zero value of f's return type
 * is returned.
 *
 * @tparam Cases... a set of VariantView types to be handled, each one having a member value to be compared against type_id
 * @param type_id the selector value that identifies a particular Case
 * @param f a callable to be invoked on matching views, or a special dummy type-identifier
 * @param variants... a (possibly empty) set of Variants to handle
 * @return whatever f returns
 */
template <class... Cases, class F, class... Variants>
constexpr auto ApplyFunctor(TypeId type_id, F &&f, Variants &&... variants)
{
  return pack::ApplyT<type_util::Switch, pack::ConcatT<Cases...>>::Invoke(
      type_id, std::forward<F>(f), std::forward<Variants>(variants)...);
}

// A specialization of ApplyFunctor for integral types only (Int* and UInt*).
template <class F, class... Variants>
constexpr auto ApplyIntegralFunctor(TypeId type_id, F &&f, Variants &&... variants)
{
  return ApplyFunctor<IntegralTypes>(type_id, std::forward<F>(f),
                                     std::forward<Variants>(variants)...);
}

// A specialization of ApplyFunctor for numeric types only (Int*, UInt*, Float*, Fixed*).
template <class F, class... Variants>
constexpr auto ApplyNumericFunctor(TypeId type_id, F &&f, Variants &&... variants)
{
  return ApplyFunctor<NumericTypes>(type_id, std::forward<F>(f),
                                    std::forward<Variants>(variants)...);
}

// A specialization of ApplyFunctor for primitive types only (Bool, Int*, UInt*, Float*, Fixed*).
template <class F, class... Variants>
constexpr auto ApplyPrimitiveFunctor(TypeId type_id, F &&f, Variants &&... variants)
{
  return ApplyFunctor<PrimitiveTypes>(type_id, std::forward<F>(f),
                                      std::forward<Variants>(variants)...);
}

// A specialization of ApplyFunctor for all builtin types(Bool, Int*, UInt*, Float*, Fixed*,
// String and Adress).
template <class F, class... Variants>
constexpr auto ApplyBuiltinFunctor(TypeId type_id, F &&f, Variants &&... variants)
{
  return ApplyFunctor<BuiltinTypes>(type_id, std::forward<F>(f),
                                    std::forward<Variants>(variants)...);
}

// Slots can be used to construct anonymous callables for ApplyFunctor
// that handle different type_ids _differently_.
//
// Example:
//
//     TypeId type_id = TypeIds::Int32;
//
//     ApplyFunctor<SignedIntegerTypes>(type_id,
//         Slots(
//             TypeIdSlot<VariantView<TypeIds::Int8>, VariantView<TypeIds::Int16>>(
//                 [](auto){ return std::string{"This is a small integer."}; }),
//
//             TypeIdSlot<VariantView<TypeIds::Int32>, VariantView<TypeIds::Int64>>(
//                 [](auto){ return std::string{"This is a large integer."}; })
//         ));
//                 
/**
 * Slot that handles Views by passing constructed views to f.
 *
 * @tparam Views... a set of VariantViews to be matched against type_id in ApplyFunctor invocation
 * @param f a callable at the core of this slot
 * @return slot
 */
template <class... Views, class F>
constexpr auto TypeIdSlot(F &&f)
{
  return pack::ApplyT<value_util::SlotType, pack::ConcatT<F, Views...>>(std::forward<F>(f));
}

/**
 * Slot that handles Views by passing constructed views to f.
 * It implicitly appends DefaultVariantView to handled Views.
 *
 * @tparam Views... a set of VariantViews to be matched against type_id in ApplyFunctor invocation
 * @param f a callable at the core of this slot
 * @return slot
 */
template <class... Views, class F>
constexpr auto DefaultSlot(F &&f)
{
  return pack::ApplyT<value_util::SlotType, pack::ConcatT<F, Views..., DefaultVariantView>>(
      std::forward<F>(f));
}

/**
 * Slot that handles Views of integral types by passing constructed views to f.
 *
 * @param f a callable at the core of this slot
 * @return slot
 */
template <class F>
constexpr auto IntegralSlot(F &&f)
{
  return TypeIdSlot<IntegralTypes>(std::forward<F>(f));
}

/**
 * Slot that handles Views of numeric types by passing constructed views to f.
 *
 * @param f a callable at the core of this slot
 * @return slot
 */
template <class F>
constexpr auto NumericSlot(F &&f)
{
  return TypeIdSlot<NumericTypes>(std::forward<F>(f));
}

/**
 * Slot that handles Views of primitive types by passing constructed views to f.
 *
 * @param f a callable at the core of this slot
 * @return slot
 */
template <class F>
constexpr auto PrimitiveSlot(F &&f)
{
  return TypeIdSlot<PrimitiveTypes>(std::forward<F>(f));
}

/**
 * Slot that handles Views of all builtin types by passing constructed views to f.
 *
 * @param f a callable at the core of this slot
 * @return slot
 */
template <class F>
constexpr auto BuiltinSlot(F &&f)
{
  return TypeIdSlot<BuiltinTypes>(std::forward<F>(f));
}

/**
 * Slot that handles dummy type arguments in zero-variant ApplyFunctor invocations.
 * Unlike in a non-nullary slot, its type parameters should be instances of IdToType, to match
 * a dummy argument passed by ApplyFunctor in case of type_id match.
 *
 * @tparam TypeIdValues... a set of TypeIds to be matched against type_id in ApplyFunctor invocation
 * @param f a callable at the core of this slot
 * @return slot
 */
template <class... TypeIdValues, class F>
constexpr auto NullarySlot(F &&f)
{
  return TypeIdSlot<TypeIdValues...>(std::forward<F>(f));
}

/**
 * Slot that handles dummy type arguments in zero-variant ApplyFunctor invocations.
 * It implicitly appends a dummy type that DefaultObjectCase passes for nullary invocations.
 * Unlike in a non-nullary slot, its type parameters should be instances of IdToType, to match
 * a dummy argument passed by ApplyFunctor in case of type_id match.
 *
 * @tparam TypeIdValues... a set of TypeIds to be matched against type_id in ApplyFunctor invocation
 * @param f a callable at the core of this slot
 * @return slot
 */
template <class... TypeIdValues, class F>
constexpr auto DefaultNullarySlot(F &&f)
{
  return NullarySlot<TypeIdValues..., VariantValue<TypeIds::NumReserved, Ptr<Object>>>(
      std::forward<F>(f));
}

/**
 * Slot that handles dummy type arguments of integral id-types in zero-variant invocations.
 *
 * @param f a callable at the core of this slot
 * @return slot
 */
template <class F>
constexpr auto IntegralNullarySlot(F &&f)
{
  return NullarySlot<IntegralTypeIds>(std::forward<F>(f));
}

/**
 * Slot that handles dummy type arguments of numeric id-types in zero-variant invocations.
 *
 * @param f a callable at the core of this slot
 * @return slot
 */
template <class F>
constexpr auto NumericNullarySlot(F &&f)
{
  return NullarySlot<NumericTypeIds>(std::forward<F>(f));
}

/**
 * Slot that handles dummy type arguments of primitive id-types in zero-variant invocations.
 *
 * @param f a callable at the core of this slot
 * @return slot
 */
template <class F>
constexpr auto PrimitiveNullarySlot(F &&f)
{
  return NullarySlot<PrimitiveTypeIds>(std::forward<F>(f));
}

/**
 * Slot that handles dummy type arguments of all builtin id-types in zero-variant invocations.
 *
 * @param f a callable at the core of this slot
 * @return slot
 */
template <class F>
constexpr auto BuiltinNullarySlot(F &&f)
{
  return NullarySlot<BuiltinTypeIds>(std::forward<F>(f));
}

}  // namespace vm
}  // namespace fetch
