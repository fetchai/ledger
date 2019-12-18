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

#include <cstddef>
#include <cstdint>
#include <tuple>

namespace fetch {
namespace vm {

class IPair : public Object
{
public:
  IPair()           = delete;
  ~IPair() override = default;
  static Ptr<IPair>           Constructor(VM *vm, TypeId type_id);
  virtual TemplateParameter1 First()                    = 0;
  virtual TemplateParameter2 Second()                    = 0;

    template <typename Key, template <typename, typename> class Container>
    static Ptr<IPair> inner(TypeId value_type_id, VM *vm, TypeId type_id);

    static inline Ptr<IPair> outer(TypeId key_type_id, TypeId value_type_id, VM *vm, TypeId type_id);

protected:
  IPair(VM *vm, TypeId type_id)
    : Object(vm, type_id)
  {}
};

template <typename Key, typename Value>
struct Pair : public IPair
{
  Pair(VM *vm, TypeId type_id)
    : IPair(vm, type_id)
  {}
  ~Pair() override = default;

  TemplateParameter1 First() override
  {
      TemplateParameter1 &value = pair.first;
      return value;
  }

    TemplateParameter2 Second() override
    {
      TemplateParameter2 &value = pair.second;
      return value;
    }

  bool SerializeTo(MsgPackSerializer &buffer) override
  {
    auto constructor = buffer.NewPairConstructor();
    auto pair_ser     = constructor(0);

    auto const &v = pair;

    auto f1 = [&v, this](MsgPackSerializer &serializer) {
        return SerializeElement<Key>(serializer, v.first);
      };

      auto f2 = [&v, this](MsgPackSerializer &serializer) {
        return SerializeElement<Value>(serializer, v.second);
      };

      if (!pair_ser.AppendUsingFunction(f1, f2))
      {
        return false;
      }

    return true;
  }

  bool DeserializeFrom(MsgPackSerializer &buffer) override
  {
    TypeInfo const &type_info     = vm_->GetTypeInfo(GetTypeId());
    TypeId const    key_type_id   = type_info.template_parameter_type_ids[0];
    TypeId const    value_type_id = type_info.template_parameter_type_ids[1];

    auto pair_ser = buffer.NewPairDeserializer();
      TemplateParameter1 key;
      TemplateParameter2 value;

      auto f1 = [key_type_id, &key, this](MsgPackSerializer &serializer) {
        return DeserializeElement<Key>(key_type_id, serializer, key);
      };

      auto f2 = [value_type_id, &value, this](MsgPackSerializer &serializer) {
        return DeserializeElement<Value>(value_type_id, serializer, value);
      };

      if (!pair_ser.GetNextKeyPairUsingFunction(f1, f2))
      {
        return false;
      }

      pair.first=key;
      pair.second=value;

    return true;
  }

  std::pair<TemplateParameter1, TemplateParameter2> pair;

private:
  template <typename U, typename TemplateParameterType>
  std::enable_if_t<IsPtr<U>::value, bool> SerializeElement(MsgPackSerializer &          buffer,
                                                           TemplateParameterType const &v)
  {
    if (v.object == nullptr)
    {
      RuntimeError("Cannot serialise null reference element in " + GetTypeName());
      return false;
    }

    return v.object->SerializeTo(buffer);
  }

  template <typename U, typename TemplateParameterType>
  std::enable_if_t<IsPrimitive<U>::value, bool> SerializeElement(MsgPackSerializer &buffer,
                                                                 TemplateParameterType const &v)
  {
    buffer << v.template Get<U>();
    return true;
  }

  template <typename U, typename TemplateParameterType>
  std::enable_if_t<IsPtr<U>::value, bool> DeserializeElement(TypeId                 type_id,
                                                             MsgPackSerializer &    buffer,
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
  std::enable_if_t<IsPrimitive<U>::value, bool> DeserializeElement(TypeId                 type_id,
                                                                   MsgPackSerializer &    buffer,
                                                                   TemplateParameterType &v)
  {
    U data;
    buffer >> data;
    v.Construct(data, type_id);
    return true;
  }
};

template <typename Key, template <typename, typename> class Container = Pair>
Ptr<IPair> IPair::inner(TypeId value_type_id, VM *vm, TypeId type_id)
{
  switch (value_type_id)
  {
  case TypeIds::Bool:
  {
    return Ptr<IPair>(new Container<Key, uint8_t>(vm, type_id));
  }
  case TypeIds::Int8:
  {
    return Ptr<IPair>(new Container<Key, int8_t>(vm, type_id));
  }
  case TypeIds::UInt8:
  {
    return Ptr<IPair>(new Container<Key, uint8_t>(vm, type_id));
  }
  case TypeIds::Int16:
  {
    return Ptr<IPair>(new Container<Key, int16_t>(vm, type_id));
  }
  case TypeIds::UInt16:
  {
    return Ptr<IPair>(new Container<Key, uint16_t>(vm, type_id));
  }
  case TypeIds::Int32:
  {
    return Ptr<IPair>(new Container<Key, int32_t>(vm, type_id));
  }
  case TypeIds::UInt32:
  {
    return Ptr<IPair>(new Container<Key, uint32_t>(vm, type_id));
  }
  case TypeIds::Int64:
  {
    return Ptr<IPair>(new Container<Key, int64_t>(vm, type_id));
  }
  case TypeIds::UInt64:
  {
    return Ptr<IPair>(new Container<Key, uint64_t>(vm, type_id));
  }
  case TypeIds::Fixed32:
  {
    return Ptr<IPair>(new Container<Key, fixed_point::fp32_t>(vm, type_id));
  }
  case TypeIds::Fixed64:
  {
    return Ptr<IPair>(new Container<Key, fixed_point::fp64_t>(vm, type_id));
  }
  default:
  {
    return Ptr<IPair>(new Container<Key, Ptr<Object>>(vm, type_id));
  }
  }  // switch
}

inline Ptr<IPair> IPair::outer(TypeId key_type_id, TypeId value_type_id, VM *vm, TypeId type_id)
{
  switch (key_type_id)
  {
  case TypeIds::Bool:
  {
    return inner<uint8_t,Pair>(value_type_id, vm, type_id);
  }
  case TypeIds::Int8:
  {
    return inner<int8_t,Pair>(value_type_id, vm, type_id);
  }
  case TypeIds::UInt8:
  {
    return inner<uint8_t,Pair>(value_type_id, vm, type_id);
  }
  case TypeIds::Int16:
  {
    return inner<int16_t,Pair>(value_type_id, vm, type_id);
  }
  case TypeIds::UInt16:
  {
    return inner<uint16_t,Pair>(value_type_id, vm, type_id);
  }
  case TypeIds::Int32:
  {
    return inner<int32_t,Pair>(value_type_id, vm, type_id);
  }
  case TypeIds::UInt32:
  {
    return inner<uint32_t,Pair>(value_type_id, vm, type_id);
  }
  case TypeIds::Int64:
  {
    return inner<int64_t,Pair>(value_type_id, vm, type_id);
  }
  case TypeIds::UInt64:
  {
    return inner<uint64_t,Pair>(value_type_id, vm, type_id);
  }
  case TypeIds::Fixed32:
  {
    return inner<fixed_point::fp32_t,Pair>(value_type_id, vm, type_id);
  }
  case TypeIds::Fixed64:
  {
    return inner<fixed_point::fp64_t,Pair>(value_type_id, vm, type_id);
  }
  default:
  {
    return inner<Ptr<Object>,Pair>(value_type_id, vm, type_id);
  }
  }  // switch
}

inline Ptr<IPair> IPair::Constructor(VM *vm, TypeId type_id)
{
  TypeInfo const &type_info     = vm->GetTypeInfo(type_id);
  TypeId const    key_type_id   = type_info.template_parameter_type_ids[0];
  TypeId const    value_type_id = type_info.template_parameter_type_ids[1];
  return outer(key_type_id, value_type_id, vm, type_id);
}

}  // namespace vm
}  // namespace fetch
