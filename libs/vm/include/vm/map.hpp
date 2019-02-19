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

#include "vm/vm.hpp"

namespace fetch {
namespace vm {

class IMap : public Object
{
public:
  IMap()          = delete;
  virtual ~IMap() = default;
  static Ptr<IMap> Constructor(VM *vm, TypeId type_id);
  virtual int32_t  Count() const = 0;

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
  size_t operator()(Variant const &v) const
  {
    return std::hash<T>()(v.primitive.Get<T>());
  }
};
template <typename T>
struct H<T, typename std::enable_if_t<IsPtr<T>::value>>
{
  size_t operator()(Variant const &v) const
  {
    return v.object->GetHashCode();
  }
};

template <typename T, typename = void>
struct E;
template <typename T>
struct E<T, typename std::enable_if_t<IsPrimitive<T>::value>>
{
  bool operator()(Variant const &lhsv, Variant const &rhsv) const
  {
    return lhsv.primitive.Get<T>() == rhsv.primitive.Get<T>();
  }
};
template <typename T>
struct E<T, typename std::enable_if_t<IsPtr<T>::value>>
{
  bool operator()(Variant const &lhsv, Variant const &rhsv) const
  {
    return lhsv.object->Equals(lhsv.object, rhsv.object);
  }
};

template <typename Key, typename Value>
struct Map : public IMap
{
  using Pair = std::pair<Variant, Variant>;

  Map()          = delete;
  virtual ~Map() = default;

  Map(VM *vm, TypeId type_id)
    : IMap(vm, type_id)
  {}

  virtual int32_t Count() const override
  {
    return int32_t(map.size());
  }

  Value *Find(Variant &keyv)
  {
    auto it = map.find(keyv);
    if (it != map.end())
    {
      keyv.Reset();
      void *ptr = &(it->second);
      return static_cast<Value *>(ptr);
    }
    RuntimeError("map key does not exist");
    return nullptr;
  }

  template <typename U>
  typename std::enable_if_t<IsPrimitive<U>::value, Value *> Find()
  {
    Variant &keyv = Pop();
    return Find(keyv);
  }

  template <typename U>
  typename std::enable_if_t<IsPtr<U>::value, Value *> Find()
  {
    Variant &keyv = Pop();
    if (keyv.object)
    {
      return Find(keyv);
    }
    RuntimeError("map key is null reference");
    return nullptr;
  }

  virtual void *FindElement() override
  {
    return Find<Key>();
  }

  virtual void PushElement(TypeId element_type_id) override
  {
    Value *ptr = Find<Key>();
    if (ptr)
    {
      Variant &top = Push();
      top.Construct(*ptr, element_type_id);
    }
  }

  template <typename U>
  typename std::enable_if_t<IsPrimitive<U>::value, void> Store(Variant &keyv, Variant &valuev)
  {
    map.insert(Pair(keyv, valuev));
  }

  template <typename U>
  typename std::enable_if_t<IsPtr<U>::value, void> Store(Variant &keyv, Variant &valuev)
  {
    if (keyv.object)
    {
      map.insert(Pair(keyv, valuev));
      return;
    }
    RuntimeError("map key is null reference");
  }

  virtual void PopToElement() override
  {
    Variant &keyv   = Pop();
    Variant &valuev = Pop();
    Store<Key>(keyv, valuev);
    valuev.Reset();
    keyv.Reset();
  }

  std::unordered_map<Variant, Variant, H<Key>, E<Key>> map;
};

template <typename Key>
inline Ptr<IMap> inner(TypeId value_type_id, VM *vm, TypeId type_id)
{
  switch (value_type_id)
  {
  case TypeIds::Bool:
  {
    return Ptr<IMap>(new Map<Key, uint8_t>(vm, type_id));
  }
  case TypeIds::Int8:
  {
    return Ptr<IMap>(new Map<Key, int8_t>(vm, type_id));
  }
  case TypeIds::Byte:
  {
    return Ptr<IMap>(new Map<Key, uint8_t>(vm, type_id));
  }
  case TypeIds::Int16:
  {
    return Ptr<IMap>(new Map<Key, int16_t>(vm, type_id));
  }
  case TypeIds::UInt16:
  {
    return Ptr<IMap>(new Map<Key, uint16_t>(vm, type_id));
  }
  case TypeIds::Int32:
  {
    return Ptr<IMap>(new Map<Key, int32_t>(vm, type_id));
  }
  case TypeIds::UInt32:
  {
    return Ptr<IMap>(new Map<Key, uint32_t>(vm, type_id));
  }
  case TypeIds::Int64:
  {
    return Ptr<IMap>(new Map<Key, int64_t>(vm, type_id));
  }
  case TypeIds::UInt64:
  {
    return Ptr<IMap>(new Map<Key, uint64_t>(vm, type_id));
  }
  case TypeIds::Float32:
  {
    return Ptr<IMap>(new Map<Key, float>(vm, type_id));
  }
  case TypeIds::Float64:
  {
    return Ptr<IMap>(new Map<Key, double>(vm, type_id));
  }
  default:
  {
    return Ptr<IMap>(new Map<Key, Ptr<Object>>(vm, type_id));
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
  case TypeIds::Byte:
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
