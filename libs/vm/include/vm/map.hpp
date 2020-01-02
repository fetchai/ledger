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

#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/vm.hpp"

#include <cstddef>
#include <cstdint>
#include <map>

namespace fetch {
namespace vm {

class IMap : public Object
{
public:
  IMap()           = delete;
  ~IMap() override = default;
  static Ptr<IMap>           Constructor(VM *vm, TypeId type_id);
  virtual int32_t            Count() const                                                     = 0;
  virtual TemplateParameter2 GetIndexedValue(TemplateParameter1 const &key)                    = 0;
  virtual void SetIndexedValue(TemplateParameter1 const &key, TemplateParameter2 const &value) = 0;

  template <typename Key, template <typename, typename> class Container>
  static Ptr<IMap> inner(TypeId value_type_id, VM *vm, TypeId type_id);

  static inline Ptr<IMap> outer(TypeId key_type_id, TypeId value_type_id, VM *vm, TypeId type_id);

protected:
  IMap(VM *vm, TypeId type_id)
    : Object(vm, type_id)
  {}
};

template <typename T, typename = void>
struct MapComparator;

template <typename T>
struct MapComparator<T, IfIsPrimitive<T>>
{
  constexpr bool operator()(fetch::vm::TemplateParameter1 const &lhs,
                            fetch::vm::TemplateParameter1 const &rhs) const
  {
    return lhs.primitive.Get<T>() < rhs.primitive.Get<T>();
  }
};

template <typename T>
struct MapComparator<T, IfIsPtr<T>>
{
  constexpr bool operator()(fetch::vm::TemplateParameter1 const &lhs,
                            fetch::vm::TemplateParameter1 const &rhs) const
  {
    return lhs.object->IsLessThan(lhs.object, rhs.object);
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
  IfIsPrimitive<U, TemplateParameter2 *> Get(TemplateParameter1 const &key)
  {
    return Find(key);
  }

  template <typename U>
  IfIsPtr<U, TemplateParameter2 *> Get(TemplateParameter1 const &key)
  {
    if (key.object)
    {
      return Find(key);
    }
    RuntimeError("map key is null reference");
    return nullptr;
  }

  TemplateParameter2 GetIndexedValue(TemplateParameter1 const &key) override
  {
    TemplateParameter2 *ptr = Get<Key>(key);
    if (ptr != nullptr)
    {
      return *ptr;
    }
    // Not found
    return TemplateParameter2();
  }

  template <typename U>
  IfIsPrimitive<U> Store(TemplateParameter1 const &key, TemplateParameter2 const &value)
  {
    map[key] = value;
  }

  template <typename U>
  IfIsPtr<U> Store(TemplateParameter1 const &key, TemplateParameter2 const &value)
  {
    if (key.object)
    {
      map[key] = value;
      return;
    }
    RuntimeError("map key is null reference");
  }

  void SetIndexedValue(TemplateParameter1 const &key, TemplateParameter2 const &value) override
  {
    Store<Key>(key, value);
  }

  bool SerializeTo(MsgPackSerializer &buffer) override
  {
    auto constructor = buffer.NewMapConstructor();
    auto map_ser     = constructor(map.size());

    for (auto const &v : map)
    {
      auto f1 = [&v, this](MsgPackSerializer &serializer) {
        return SerializeElement<Key>(serializer, v.first);
      };

      auto f2 = [&v, this](MsgPackSerializer &serializer) {
        return SerializeElement<Value>(serializer, v.second);
      };

      if (!map_ser.AppendUsingFunction(f1, f2))
      {
        return false;
      }
    }

    return true;
  }

  bool DeserializeFrom(MsgPackSerializer &buffer) override
  {
    TypeInfo const &type_info     = vm_->GetTypeInfo(GetTypeId());
    TypeId const    key_type_id   = type_info.template_parameter_type_ids[0];
    TypeId const    value_type_id = type_info.template_parameter_type_ids[1];

    auto map_ser = buffer.NewMapDeserializer();
    for (uint64_t i = 0; i < map_ser.size(); ++i)
    {
      TemplateParameter1 key;
      TemplateParameter2 value;

      auto f1 = [key_type_id, &key, this](MsgPackSerializer &serializer) {
        return DeserializeElement<Key>(key_type_id, serializer, key);
      };

      auto f2 = [value_type_id, &value, this](MsgPackSerializer &serializer) {
        return DeserializeElement<Value>(value_type_id, serializer, value);
      };

      if (!map_ser.GetNextKeyPairUsingFunction(f1, f2))
      {
        return false;
      }

      map.insert({key, value});
    }

    return true;
  }

  std::map<TemplateParameter1, TemplateParameter2, MapComparator<Key>> map;

private:
  template <typename U, typename TemplateParameterType>
  IfIsPtr<U, bool> SerializeElement(MsgPackSerializer &buffer, TemplateParameterType const &v)
  {
    if (v.object == nullptr)
    {
      RuntimeError("Cannot serialise null reference element in " + GetTypeName());
      return false;
    }

    return v.object->SerializeTo(buffer);
  }

  template <typename U, typename TemplateParameterType>
  IfIsPrimitive<U, bool> SerializeElement(MsgPackSerializer &buffer, TemplateParameterType const &v)
  {
    buffer << v.template Get<U>();
    return true;
  }

  template <typename U, typename TemplateParameterType>
  IfIsPtr<U, bool> DeserializeElement(TypeId type_id, MsgPackSerializer &buffer,
                                      TemplateParameterType &v)
  {
    if (!vm_->IsDefaultSerializeConstructable(type_id))
    {
      vm_->RuntimeError("Cannot deserialize type " + vm_->GetTypeName(type_id) +
                        " as no serialisation constructor exists.");
      return false;
    }

    v.Construct(vm_->DefaultSerializeConstruct(type_id), type_id);
    return v.object->DeserializeFrom(buffer);
  }

  template <typename U, typename TemplateParameterType>
  IfIsPrimitive<U, bool> DeserializeElement(TypeId type_id,

                                            MsgPackSerializer &buffer, TemplateParameterType &v)
  {
    U data;
    buffer >> data;
    v.Construct(data, type_id);
    return true;
  }
};

template <typename Key, template <typename, typename> class Container = Map>
Ptr<IMap> IMap::inner(TypeId value_type_id, VM *vm, TypeId type_id)
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

inline Ptr<IMap> IMap::outer(TypeId key_type_id, TypeId value_type_id, VM *vm, TypeId type_id)
{
  switch (key_type_id)
  {
  case TypeIds::Bool:
  {
    return inner<uint8_t, Map>(value_type_id, vm, type_id);
  }
  case TypeIds::Int8:
  {
    return inner<int8_t, Map>(value_type_id, vm, type_id);
  }
  case TypeIds::UInt8:
  {
    return inner<uint8_t, Map>(value_type_id, vm, type_id);
  }
  case TypeIds::Int16:
  {
    return inner<int16_t, Map>(value_type_id, vm, type_id);
  }
  case TypeIds::UInt16:
  {
    return inner<uint16_t, Map>(value_type_id, vm, type_id);
  }
  case TypeIds::Int32:
  {
    return inner<int32_t, Map>(value_type_id, vm, type_id);
  }
  case TypeIds::UInt32:
  {
    return inner<uint32_t, Map>(value_type_id, vm, type_id);
  }
  case TypeIds::Int64:
  {
    return inner<int64_t, Map>(value_type_id, vm, type_id);
  }
  case TypeIds::UInt64:
  {
    return inner<uint64_t, Map>(value_type_id, vm, type_id);
  }
  case TypeIds::Fixed32:
  {
    return inner<fixed_point::fp32_t, Map>(value_type_id, vm, type_id);
  }
  case TypeIds::Fixed64:
  {
    return inner<fixed_point::fp64_t, Map>(value_type_id, vm, type_id);
  }
  default:
  {
    return inner<Ptr<Object>, Map>(value_type_id, vm, type_id);
  }
  }  // switch
}

inline Ptr<IMap> IMap::Constructor(VM *vm, TypeId type_id)
{
  TypeInfo const &type_info     = vm->GetTypeInfo(type_id);
  TypeId const    key_type_id   = type_info.template_parameter_type_ids[0];
  TypeId const    value_type_id = type_info.template_parameter_type_ids[1];
  return outer(key_type_id, value_type_id, vm, type_id);
}

}  // namespace vm
}  // namespace fetch
