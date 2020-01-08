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
#include <tuple>

namespace fetch {
namespace vm {

class IPair : public Object
{
public:
  IPair()           = delete;
  ~IPair() override = default;
  static Ptr<IPair>          Constructor(VM *vm, TypeId type_id);
  virtual TemplateParameter1 GetFirst()                                  = 0;
  virtual TemplateParameter2 GetSecond()                                 = 0;
  virtual void               SetFirst(TemplateParameter1 const &first)   = 0;
  virtual void               SetSecond(TemplateParameter2 const &second) = 0;

  template <typename FirstType, template <typename, typename> class Container>
  static Ptr<IPair> inner(TypeId second_type_id, VM *vm, TypeId type_id);

  static inline Ptr<IPair> outer(TypeId first_type_id, TypeId second_type_id, VM *vm,
                                 TypeId type_id);

protected:
  IPair(VM *vm, TypeId type_id)
    : Object(vm, type_id)
  {}
};

template <typename FirstType, typename SecondType>
struct Pair : public IPair
{
  Pair(VM *vm, TypeId type_id)
    : IPair(vm, type_id)
  {}

  ~Pair() override = default;

  TemplateParameter1 GetFirst() override
  {
    TemplateParameter1 &second = pair.first;
    return second;
  }

  TemplateParameter2 GetSecond() override
  {
    TemplateParameter2 &second = pair.second;
    return second;
  }

  void SetFirst(TemplateParameter1 const &first) override
  {
    pair.first = first;
  }

  void SetSecond(TemplateParameter2 const &second) override
  {
    pair.second = second;
  }

  bool SerializeTo(MsgPackSerializer &buffer) override
  {
    auto constructor = buffer.NewPairConstructor();

    // 0 = no element, 1 = first only, 2 = second only, 3 = both
    uint64_t size{0};

    // If first is available size is 1
    if (pair.first.object != nullptr)
    {
      size = 1;
    }

    if (pair.second.object != nullptr)
    {
      // If first was available size is be 3
      if (size == 1)
      {
        size = 3;
      }
      // If only second is available size is 2
      else
      {
        size = 2;
      }
    }

    auto pair_ser = constructor(size);

    auto const &v = pair;

    auto f1 = [&v, this](MsgPackSerializer &serializer) {
      return SerializeElement<FirstType>(serializer, v.first);
    };

    auto f2 = [&v, this](MsgPackSerializer &serializer) {
      return SerializeElement<SecondType>(serializer, v.second);
    };

    // Append first if first or both are available
    if (size == 1 || size == 3)
    {
      if (!pair_ser.AppendFirst(f1))
      {
        return false;
      }
    }

    // Append second if second or both are available
    if (size == 2 || size == 3)
    {
      if (!pair_ser.AppendSecond(f2))
      {
        return false;
      }
    }

    return true;
  }

  bool DeserializeFrom(MsgPackSerializer &buffer) override
  {
    TypeInfo const &type_info      = vm_->GetTypeInfo(GetTypeId());
    TypeId const    first_type_id  = type_info.template_parameter_type_ids[0];
    TypeId const    second_type_id = type_info.template_parameter_type_ids[1];

    auto               pair_ser = buffer.NewPairDeserializer();
    TemplateParameter1 first;
    TemplateParameter2 second;

    auto f1 = [first_type_id, &first, this](MsgPackSerializer &serializer) {
      return DeserializeElement<FirstType>(first_type_id, serializer, first);
    };

    auto f2 = [second_type_id, &second, this](MsgPackSerializer &serializer) {
      return DeserializeElement<SecondType>(second_type_id, serializer, second);
    };

    if (pair_ser.size() > 3)
    {
      RuntimeError("Deserialised pair has wrong size");
    }

    if (pair_ser.size() == 1 || pair_ser.size() == 3)
    {
      if (!pair_ser.GetFirstUsingFunction(f1))
      {
        return false;
      }
      pair.first = first;
    }

    if (pair_ser.size() == 2 || pair_ser.size() == 3)
    {
      if (!pair_ser.GetSecondUsingFunction(f2))
      {
        return false;
      }
      pair.second = second;
    }

    return true;
  }

  std::pair<TemplateParameter1, TemplateParameter2> pair;

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
  IfIsPrimitive<U, bool> DeserializeElement(TypeId type_id, MsgPackSerializer &buffer,
                                            TemplateParameterType &v)
  {
    U data;
    buffer >> data;
    v.Construct(data, type_id);
    return true;
  }
};

template <typename FirstType, template <typename, typename> class Container = Pair>
Ptr<IPair> IPair::inner(TypeId second_type_id, VM *vm, TypeId type_id)
{
  switch (second_type_id)
  {
  case TypeIds::Bool:
  {
    return Ptr<IPair>(new Container<FirstType, uint8_t>(vm, type_id));
  }
  case TypeIds::Int8:
  {
    return Ptr<IPair>(new Container<FirstType, int8_t>(vm, type_id));
  }
  case TypeIds::UInt8:
  {
    return Ptr<IPair>(new Container<FirstType, uint8_t>(vm, type_id));
  }
  case TypeIds::Int16:
  {
    return Ptr<IPair>(new Container<FirstType, int16_t>(vm, type_id));
  }
  case TypeIds::UInt16:
  {
    return Ptr<IPair>(new Container<FirstType, uint16_t>(vm, type_id));
  }
  case TypeIds::Int32:
  {
    return Ptr<IPair>(new Container<FirstType, int32_t>(vm, type_id));
  }
  case TypeIds::UInt32:
  {
    return Ptr<IPair>(new Container<FirstType, uint32_t>(vm, type_id));
  }
  case TypeIds::Int64:
  {
    return Ptr<IPair>(new Container<FirstType, int64_t>(vm, type_id));
  }
  case TypeIds::UInt64:
  {
    return Ptr<IPair>(new Container<FirstType, uint64_t>(vm, type_id));
  }
  case TypeIds::Fixed32:
  {
    return Ptr<IPair>(new Container<FirstType, fixed_point::fp32_t>(vm, type_id));
  }
  case TypeIds::Fixed64:
  {
    return Ptr<IPair>(new Container<FirstType, fixed_point::fp64_t>(vm, type_id));
  }
  default:
  {
    return Ptr<IPair>(new Container<FirstType, Ptr<Object>>(vm, type_id));
  }
  }  // switch
}

inline Ptr<IPair> IPair::outer(TypeId first_type_id, TypeId second_type_id, VM *vm, TypeId type_id)
{
  switch (first_type_id)
  {
  case TypeIds::Bool:
  {
    return inner<uint8_t, Pair>(second_type_id, vm, type_id);
  }
  case TypeIds::Int8:
  {
    return inner<int8_t, Pair>(second_type_id, vm, type_id);
  }
  case TypeIds::UInt8:
  {
    return inner<uint8_t, Pair>(second_type_id, vm, type_id);
  }
  case TypeIds::Int16:
  {
    return inner<int16_t, Pair>(second_type_id, vm, type_id);
  }
  case TypeIds::UInt16:
  {
    return inner<uint16_t, Pair>(second_type_id, vm, type_id);
  }
  case TypeIds::Int32:
  {
    return inner<int32_t, Pair>(second_type_id, vm, type_id);
  }
  case TypeIds::UInt32:
  {
    return inner<uint32_t, Pair>(second_type_id, vm, type_id);
  }
  case TypeIds::Int64:
  {
    return inner<int64_t, Pair>(second_type_id, vm, type_id);
  }
  case TypeIds::UInt64:
  {
    return inner<uint64_t, Pair>(second_type_id, vm, type_id);
  }
  case TypeIds::Fixed32:
  {
    return inner<fixed_point::fp32_t, Pair>(second_type_id, vm, type_id);
  }
  case TypeIds::Fixed64:
  {
    return inner<fixed_point::fp64_t, Pair>(second_type_id, vm, type_id);
  }
  default:
  {
    return inner<Ptr<Object>, Pair>(second_type_id, vm, type_id);
  }
  }  // switch
}

inline Ptr<IPair> IPair::Constructor(VM *vm, TypeId type_id)
{
  TypeInfo const &type_info      = vm->GetTypeInfo(type_id);
  TypeId const    first_type_id  = type_info.template_parameter_type_ids[0];
  TypeId const    second_type_id = type_info.template_parameter_type_ids[1];
  return outer(first_type_id, second_type_id, vm, type_id);
}

}  // namespace vm
}  // namespace fetch
