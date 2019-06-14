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
#include "vm/vm.hpp"

namespace fetch {
namespace vm {

class IMap : public Object
{
public:
  IMap()          = delete;
  virtual ~IMap() = default;
  static Ptr<IMap>           Constructor(VM *vm, TypeId type_id);
  virtual int32_t            Count() const                                                     = 0;
  virtual TemplateParameter2 GetIndexedValue(TemplateParameter1 const &key)                    = 0;
  virtual void SetIndexedValue(TemplateParameter1 const &key, TemplateParameter2 const &value) = 0;

protected:
  IMap(VM *vm, TypeId type_id)
    : Object(vm, type_id)
  {}
};

template <typename T, typename = void>
struct H;
template <typename T>
struct H<T, typename std::enable_if_t<IsPrimitive<T>::value>>
{
  size_t operator()(TemplateParameter1 const &key) const
  {
    return std::hash<T>()(key.primitive.Get<T>());
  }
};
template <typename T>
struct H<T, typename std::enable_if_t<IsPtr<T>::value>>
{
  size_t operator()(TemplateParameter1 const &key) const
  {
    return key.object->GetHashCode();
  }
};

template <typename T, typename = void>
struct E;
template <typename T>
struct E<T, typename std::enable_if_t<IsPrimitive<T>::value>>
{
  bool operator()(TemplateParameter1 const &lhs, TemplateParameter1 const &rhs) const
  {
    return math::IsEqual(lhs.primitive.Get<T>(), rhs.primitive.Get<T>());
  }
};
template <typename T>
struct E<T, typename std::enable_if_t<IsPtr<T>::value>>
{
  bool operator()(TemplateParameter1 const &lhs, TemplateParameter1 const &rhs) const
  {
    return lhs.object->IsEqual(lhs.object, rhs.object);
  }
};

template <typename Key, typename Value>
struct Map : public IMap
{
  Map(VM *vm, TypeId type_id)
    : IMap(vm, type_id)
  {}
  ~Map() override = default;

  int32_t Count() const override
  {
    return int32_t(map.size());
  }

  TemplateParameter2 *Find(TemplateParameter1 const &key)
  {
    auto it = map.find(key);
    if (it != map.end())
    {
      TemplateParameter2 &value = it->second;
      return &value;
    }
    RuntimeError("map key does not exist");
    return nullptr;
  }

  template <typename U>
  typename std::enable_if_t<IsPrimitive<U>::value, TemplateParameter2 *> Get(
      TemplateParameter1 const &key)
  {
    return Find(key);
  }

  template <typename U>
  typename std::enable_if_t<IsPtr<U>::value, TemplateParameter2 *> Get(
      TemplateParameter1 const &key)
  {
    if (key.object)
    {
      return Find(key);
    }
    RuntimeError("map key is null reference");
    return nullptr;
  }

  virtual TemplateParameter2 GetIndexedValue(TemplateParameter1 const &key) override
  {
    TemplateParameter2 *ptr = Get<Key>(key);
    if (ptr)
    {
      return *ptr;
    }
    // Not found
    return TemplateParameter2();
  }

  template <typename U>
  typename std::enable_if_t<IsPrimitive<U>::value, void> Store(TemplateParameter1 const &key,
                                                               TemplateParameter2 const &value)
  {
    map[key] = value;
  }

  template <typename U>
  typename std::enable_if_t<IsPtr<U>::value, void> Store(TemplateParameter1 const &key,
                                                         TemplateParameter2 const &value)
  {
    if (key.object)
    {
      map[key] = value;
      return;
    }
    RuntimeError("map key is null reference");
  }

  virtual void SetIndexedValue(TemplateParameter1 const &key,
                               TemplateParameter2 const &value) override
  {
    Store<Key>(key, value);
  }

  bool SerializeTo(ByteArrayBuffer &buffer) override
  {
    buffer << map;
    return true;
  }

  bool DeserializeFrom(ByteArrayBuffer &buffer) override
  {
    buffer >> map;
    return true;
  }

  std::unordered_map<TemplateParameter1, TemplateParameter2, H<Key>, E<Key>> map;
};

template <typename Key, template <typename, typename> class Container = Map>
inline Ptr<IMap> inner(TypeId value_type_id, VM *vm, TypeId type_id)
{
  switch (value_type_id)
  {
  case TypeIds::Bool:
  {
    return Ptr<IMap>(new Container<Key, uint8_t>(vm, type_id));
  }
  case TypeIds::Int8:
  {
    return Ptr<IMap>(new Container<Key, int8_t>(vm, type_id));
  }
  case TypeIds::UInt8:
  {
    return Ptr<IMap>(new Container<Key, uint8_t>(vm, type_id));
  }
  case TypeIds::Int16:
  {
    return Ptr<IMap>(new Container<Key, int16_t>(vm, type_id));
  }
  case TypeIds::UInt16:
  {
    return Ptr<IMap>(new Container<Key, uint16_t>(vm, type_id));
  }
  case TypeIds::Int32:
  {
    return Ptr<IMap>(new Container<Key, int32_t>(vm, type_id));
  }
  case TypeIds::UInt32:
  {
    return Ptr<IMap>(new Container<Key, uint32_t>(vm, type_id));
  }
  case TypeIds::Int64:
  {
    return Ptr<IMap>(new Container<Key, int64_t>(vm, type_id));
  }
  case TypeIds::UInt64:
  {
    return Ptr<IMap>(new Container<Key, uint64_t>(vm, type_id));
  }
  case TypeIds::Float32:
  {
    return Ptr<IMap>(new Container<Key, float>(vm, type_id));
  }
  case TypeIds::Float64:
  {
    return Ptr<IMap>(new Container<Key, double>(vm, type_id));
  }
  case TypeIds::Fixed32:
  {
    return Ptr<IMap>(new Container<Key, fixed_point::fp32_t>(vm, type_id));
  }
  case TypeIds::Fixed64:
  {
    return Ptr<IMap>(new Container<Key, fixed_point::fp64_t>(vm, type_id));
  }
  default:
  {
    return Ptr<IMap>(new Container<Key, Ptr<Object>>(vm, type_id));
  }
  }  // switch
}

inline Ptr<IMap> outer(TypeId key_type_id, TypeId value_type_id, VM *vm, TypeId type_id)
{
  switch (key_type_id)
  {
  case TypeIds::Bool:
  {
    return inner<uint8_t>(value_type_id, vm, type_id);
  }
  case TypeIds::Int8:
  {
    return inner<int8_t>(value_type_id, vm, type_id);
  }
  case TypeIds::UInt8:
  {
    return inner<uint8_t>(value_type_id, vm, type_id);
  }
  case TypeIds::Int16:
  {
    return inner<int16_t>(value_type_id, vm, type_id);
  }
  case TypeIds::UInt16:
  {
    return inner<uint16_t>(value_type_id, vm, type_id);
  }
  case TypeIds::Int32:
  {
    return inner<int32_t>(value_type_id, vm, type_id);
  }
  case TypeIds::UInt32:
  {
    return inner<uint32_t>(value_type_id, vm, type_id);
  }
  case TypeIds::Int64:
  {
    return inner<int64_t>(value_type_id, vm, type_id);
  }
  case TypeIds::UInt64:
  {
    return inner<uint64_t>(value_type_id, vm, type_id);
  }
  case TypeIds::Float32:
  {
    return inner<float>(value_type_id, vm, type_id);
  }
  case TypeIds::Float64:
  {
    return inner<double>(value_type_id, vm, type_id);
  }
  // case TypeIds::Fixed32:
  // {
  //   return inner<fixed_point::fp32_t>(value_type_id, vm, type_id);
  // }
  // case TypeIds::Fixed64:
  // {
  //   return inner<fixed_point::fp64_t>(value_type_id, vm, type_id);
  // }
  default:
  {
    return inner<Ptr<Object>>(value_type_id, vm, type_id);
  }
  }  // switch
}

inline Ptr<IMap> IMap::Constructor(VM *vm, TypeId type_id)
{
  TypeInfo const &type_info     = vm->GetTypeInfo(type_id);
  TypeId const    key_type_id   = type_info.parameter_type_ids[0];
  TypeId const    value_type_id = type_info.parameter_type_ids[1];
  return outer(key_type_id, value_type_id, vm, type_id);
}

}  // namespace vm
}  // namespace fetch
