#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

  template <typename T>
  auto Get() const noexcept;

  void Set(bool value) noexcept
  {
    ui8 = uint8_t(value);
  }

  void Set(int8_t value) noexcept
  {
    i8 = value;
  }

  void Set(uint8_t value) noexcept
  {
    ui8 = value;
  }

  void Set(int16_t value) noexcept
  {
    i16 = value;
  }

  void Set(uint16_t value) noexcept
  {
    ui16 = value;
  }

  void Set(int32_t value) noexcept
  {
    i32 = value;
  }

  void Set(uint32_t value) noexcept
  {
    ui32 = value;
  }

  void Set(int64_t value) noexcept
  {
    i64 = value;
  }

  void Set(uint64_t value) noexcept
  {
    ui64 = value;
  }

  void Set(fixed_point::fp32_t const &value) noexcept
  {
    i32 = value.Data();
  }

  void Set(fixed_point::fp64_t const &value) noexcept
  {
    i64 = value.Data();
  }
};

template <>
inline auto Primitive::Get<bool>() const noexcept
{
  return bool(ui8);
}

template <>
inline auto Primitive::Get<int8_t>() const noexcept
{
  return i8;
}

template <>
inline auto Primitive::Get<uint8_t>() const noexcept
{
  return ui8;
}

template <>
inline auto Primitive::Get<int16_t>() const noexcept
{
  return i16;
}

template <>
inline auto Primitive::Get<uint16_t>() const noexcept
{
  return ui16;
}

template <>
inline auto Primitive::Get<int32_t>() const noexcept
{
  return i32;
}

template <>
inline auto Primitive::Get<uint32_t>() const noexcept
{
  return ui32;
}

template <>
inline auto Primitive::Get<int64_t>() const noexcept
{
  return i64;
}

template <>
inline auto Primitive::Get<uint64_t>() const noexcept
{
  return ui64;
}

template <>
inline auto Primitive::Get<fixed_point::fp32_t>() const noexcept
{
  return fixed_point::fp32_t::FromBase(i32);
}

template <>
inline auto Primitive::Get<fixed_point::fp64_t>() const noexcept
{
  return fixed_point::fp64_t::FromBase(i64);
}

struct Variant
{
  union
  {
    Primitive   primitive{};
    Ptr<Object> object;
  };
  TypeId type_id = TypeIds::Unknown;

  Variant() noexcept
  {
    Construct();
  }

  Variant(Variant const &other) noexcept
  {
    Construct(other);
  }

  Variant(Variant &&other) noexcept
  {
    Construct(std::move(other));
  }

  template <typename T, std::enable_if_t<IsPrimitive<T>> * = nullptr>
  Variant(T other, TypeId other_type_id) noexcept
  {
    Construct(other, other_type_id);
  }

  template <typename T, typename std::enable_if_t<IsPtr<T>> * = nullptr>
  Variant(T const &other, TypeId other_type_id) noexcept
  {
    Construct(other, other_type_id);
  }

  template <typename T, typename std::enable_if_t<IsPtr<T>> * = nullptr>
  Variant(T &&other, TypeId other_type_id) noexcept
  {
    Construct(std::forward<T>(other), other_type_id);
  }

  Variant(Primitive other, TypeId other_type_id) noexcept
  {
    Construct(other, other_type_id);
  }

  void Construct() noexcept
  {
    type_id = TypeIds::Unknown;
  }

  void Construct(Variant const &other) noexcept
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

  void Construct(Variant &&other) noexcept
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

  template <typename T>
  std::enable_if_t<IsPrimitive<T>> Construct(T other, TypeId other_type_id) noexcept
  {
    primitive.Set(other);
    type_id = other_type_id;
  }

  template <typename T>
  std::enable_if_t<IsPtr<T>> Construct(T const &other, TypeId other_type_id) noexcept
  {
    new (&object) Ptr<Object>(other);
    type_id = other_type_id;
  }

  template <typename T>
  std::enable_if_t<IsPtr<T>> Construct(T &&other, TypeId other_type_id) noexcept
  {
    new (&object) Ptr<Object>(std::forward<T>(other));
    type_id = other_type_id;
  }

  void Construct(Primitive other, TypeId other_type_id) noexcept
  {
    primitive = other;
    type_id   = other_type_id;
  }

  Variant &operator=(Variant const &other) noexcept
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

  Variant &operator=(Variant &&other) noexcept
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

  template <typename T>
  std::enable_if_t<IsPrimitive<T>> Assign(T other, TypeId other_type_id) noexcept
  {
    if (!IsPrimitive())
    {
      object.Reset();
    }
    primitive.Set(other);
    type_id = other_type_id;
  }

  template <typename T>
  std::enable_if_t<IsPtr<T>> Assign(T const &other, TypeId other_type_id) noexcept
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

  template <typename T>
  std::enable_if_t<IsPtr<T>> Assign(T &&other, TypeId other_type_id) noexcept
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

  template <typename T>
  std::enable_if_t<IsVariant<T>> Assign(T const &other, TypeId /* other_type_id */) noexcept
  {
    operator=(other);
  }

  template <typename T>
  std::enable_if_t<IsVariant<T>> Assign(T &&other, TypeId /* other_type_id */) noexcept
  {
    operator=(std::forward<T>(other));
  }

  template <typename T>
  std::enable_if_t<IsPrimitive<T>, T> Get() const noexcept
  {
    return primitive.Get<T>();
  }

  template <typename T>
  std::enable_if_t<IsPtr<T>, T> Get() const noexcept
  {
    return object;
  }

  template <typename T>
  std::enable_if_t<IsVariant<T>, T> Get() const noexcept
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
  std::enable_if_t<IsPrimitive<T>, T> Move() noexcept
  {
    type_id = TypeIds::Unknown;
    return primitive.Get<T>();
  }

  template <typename T>
  std::enable_if_t<IsPtr<T>, T> Move() noexcept
  {
    type_id = TypeIds::Unknown;
    return {std::move(object)};
  }

  template <typename T>
  constexpr std::enable_if_t<IsVariant<T>, T> Move() noexcept
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

  constexpr bool IsPrimitive() const noexcept
  {
    return type_id <= TypeIds::PrimitiveMaxId;
  }

  constexpr void Reset() noexcept
  {
    if (!IsPrimitive())
    {
      object.Reset();
    }
    type_id = TypeIds::Unknown;
  }
};
using VariantArray = std::vector<Variant>;

struct TemplateParameter1 : Variant
{
  using Variant::Variant;
};

struct TemplateParameter2 : Variant
{
  using Variant::Variant;
};

struct Any : Variant
{
  using Variant::Variant;
};

struct AnyPrimitive : Variant
{
  using Variant::Variant;
};

struct AnyInteger : Variant
{
  using Variant::Variant;
};

}  // namespace vm

namespace serializers {

template <typename D>
struct MapSerializer<fetch::vm::Variant, D>
{
public:
  using Type       = fetch::vm::Variant;
  using DriverType = D;

  static uint8_t const TYPEID    = 1;
  static uint8_t const PRIMITIVE = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &variant)
  {
    auto map = map_constructor(2);
    // TODO(tfr): This is dangerous: Type Id should never be serialized
    map.Append(TYPEID, variant.type_id);

    // primitive type variant
    if (variant.IsPrimitive())
    {
      // Since primitive is a union it suffices
      // that we store one of the values.
      uint64_t val = variant.primitive.ui64;
      map.Append(PRIMITIVE, val);
    }
    else
    {
      // not supported yet
      throw std::runtime_error{"Serialization of Variant of Object type is not supported"};
    }
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &variant)
  {
    map.ExpectKeyGetValue(TYPEID, variant.type_id);

    if (variant.IsPrimitive())
    {
      uint64_t val;
      map.ExpectKeyGetValue(PRIMITIVE, val);
      variant.primitive.ui64 = val;
    }
    else
    {
      throw std::runtime_error{"Deserialization of objects not possible for variants."};
    }
  }
};

}  // namespace serializers
}  // namespace fetch
