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

inline void Serialize(ByteArrayBuffer &buffer, Variant const &variant)
{
  buffer << variant.type_id;

  switch (variant.type_id)
  {
  case TypeIds::Bool:
  case TypeIds::UInt8:
    buffer << variant.primitive.ui8;
    break;
  case TypeIds::Int8:
    buffer << variant.primitive.i8;
    break;
  case TypeIds::Int16:
    buffer << variant.primitive.i16;
    break;
  case TypeIds::UInt16:
    buffer << variant.primitive.ui16;
    break;
  case TypeIds::Int32:
    buffer << variant.primitive.i32;
    break;
  case TypeIds::UInt32:
    buffer << variant.primitive.ui32;
    break;
  case TypeIds::Int64:
    buffer << variant.primitive.i64;
    break;
  case TypeIds::UInt64:
    buffer << variant.primitive.ui64;
    break;
  case TypeIds::Float32:
    buffer << variant.primitive.f32;
    break;
  case TypeIds::Float64:
    buffer << variant.primitive.f64;
    break;
  case TypeIds::Fixed32:
    buffer << variant.primitive.i32;
    break;
  case TypeIds::Fixed64:
    buffer << variant.primitive.i64;
    break;
  default:
    if (variant.object)
    {
      if (!variant.object->SerializeTo(buffer))
      {
        throw std::runtime_error("Unable to serialize type");
      }
    }
    break;
  }
}

inline void Deserialize(ByteArrayBuffer &buffer, Variant &variant)
{
  TypeId id;
  buffer >> id;

  if (id != variant.type_id)
  {
    throw std::runtime_error("Unable to deserialize the variant type");
  }

  switch (variant.type_id)
  {
  case TypeIds::Bool:
  case TypeIds::UInt8:
    buffer >> variant.primitive.ui8;
    break;
  case TypeIds::Int8:
    buffer >> variant.primitive.i8;
    break;
  case TypeIds::Int16:
    buffer >> variant.primitive.i16;
    break;
  case TypeIds::UInt16:
    buffer >> variant.primitive.ui16;
    break;
  case TypeIds::Int32:
    buffer >> variant.primitive.i32;
    break;
  case TypeIds::UInt32:
    buffer >> variant.primitive.ui32;
    break;
  case TypeIds::Int64:
    buffer >> variant.primitive.i64;
    break;
  case TypeIds::UInt64:
    buffer >> variant.primitive.ui64;
    break;
  case TypeIds::Float32:
    buffer >> variant.primitive.f32;
    break;
  case TypeIds::Float64:
    buffer >> variant.primitive.f64;
    break;
  case TypeIds::Fixed32:
    buffer >> variant.primitive.i32;
    break;
  case TypeIds::Fixed64:
    buffer >> variant.primitive.i64;
    break;
  default:
    if (variant.object)
    {
      if (!variant.object->DeserializeFrom(buffer))
      {
        throw std::runtime_error("Failed to the deserialize compound object");
      }
    }
    break;
  }
}

}  // namespace vm
}  // namespace fetch
