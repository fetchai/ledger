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
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/fixed.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>

namespace fetch {
namespace vm {

template <typename T, typename = void>
struct GetElementType
{
  using type = typename GetStorageType<T>::type;
};
template <typename T>
struct GetElementType<T, std::enable_if_t<std::is_same<T, bool>::value>>
{
  // ElementType must NOT be bool because std::vector<bool> is a partial specialisation
  using type = uint8_t;
};

class IArray : public Object
{
public:
  IArray()           = delete;
  ~IArray() override = default;
  static Ptr<IArray> Constructor(VM *vm, TypeId type_id, int32_t size);

  virtual int32_t            Count() const                      = 0;
  virtual void               Append(TemplateParameter1 const &) = 0;
  virtual TemplateParameter1 PopBackOne()                       = 0;
  virtual Ptr<IArray>        PopBackMany(int32_t)               = 0;
  virtual TemplateParameter1 PopFrontOne()                      = 0;
  virtual Ptr<IArray>        PopFrontMany(int32_t)              = 0;
  virtual void               Reverse()                          = 0;
  virtual void               Extend(Ptr<IArray> const &)        = 0;
  virtual void               Erase(int32_t)                     = 0;

  virtual TemplateParameter1 GetIndexedValue(AnyInteger const &index)                    = 0;
  virtual void SetIndexedValue(AnyInteger const &index, TemplateParameter1 const &value) = 0;

protected:
  IArray(VM *vm, TypeId type_id)
    : Object(vm, type_id)
  {}

  template <typename... Args>
  static Ptr<IArray> Construct(VM *vm, TypeId type_id, Args &&... args);
};

template <typename T>
struct Array : public IArray
{
  using ElementType = typename GetElementType<T>::type;

  Array()           = delete;
  ~Array() override = default;

  Array(VM *vm, TypeId type_id, TypeId element_type_id__, int32_t size)
    : IArray(vm, type_id)
    , element_type_id(element_type_id__)
    , elements(static_cast<std::size_t>(size), ElementType{})
  {}

  int32_t Count() const override
  {
    return int32_t(elements.size());
  }

  void Append(TemplateParameter1 const &element) override
  {
    if (element.type_id != element_type_id)
    {
      RuntimeError("Failed to append to Array: incompatible type");
      return;
    }
    elements.push_back(element.Get<ElementType>());
  }

  TemplateParameter1 PopBackOne() override
  {
    if (elements.empty())
    {
      RuntimeError("Failed to popBack: array is empty");
      return {};
    }

    auto element = std::move(elements[elements.size() - 1u]);

    elements.pop_back();

    return TemplateParameter1(element, element_type_id);
  }

  Ptr<IArray> PopBackMany(int32_t num_to_pop) override
  {
    if (num_to_pop < 0)
    {
      RuntimeError("Failed to popBack: argument must be non-negative");
      return {};
    }

    if (elements.size() < static_cast<std::size_t>(num_to_pop))
    {
      RuntimeError("Failed to popBack: not enough elements in array");
      return {};
    }

    auto array = new Array<ElementType>(vm_, type_id_, element_type_id, num_to_pop);

    std::move(elements.rbegin(), elements.rbegin() + num_to_pop, array->elements.rbegin());

    elements.resize(elements.size() - static_cast<std::size_t>(num_to_pop));

    return Ptr<IArray>{array};
  }

  TemplateParameter1 PopFrontOne() override
  {
    if (elements.empty())
    {
      RuntimeError("Failed to popFront: array is empty");
      return {};
    }

    auto element = std::move(elements[0]);

    // Shift remaining elements to the right
    for (std::size_t i = 1u; i < elements.size(); ++i)
    {
      elements[i - 1] = std::move(elements[i]);
    }

    elements.resize(elements.size() - 1);

    return TemplateParameter1(element, element_type_id);
  }

  Ptr<IArray> PopFrontMany(int32_t num_to_pop) override
  {
    if (num_to_pop < 0)
    {
      RuntimeError("Failed to popFront: argument must be non-negative");
      return {};
    }

    if (elements.size() < static_cast<std::size_t>(num_to_pop))
    {
      RuntimeError("Failed to popFront: not enough elements in array");
      return {};
    }

    auto array = new Array<ElementType>(vm_, type_id_, element_type_id, num_to_pop);

    std::move(elements.begin(), elements.begin() + num_to_pop, array->elements.begin());

    auto const popped_size = static_cast<std::size_t>(num_to_pop);

    // Shift remaining elements to the right
    for (auto i = popped_size; i < elements.size(); ++i)
    {
      elements[i - popped_size] = std::move(elements[i]);
    }

    elements.resize(elements.size() - popped_size);

    return Ptr<IArray>{array};
  }

  void Reverse() override
  {
    std::reverse(elements.begin(), elements.end());
  }

  void Extend(Ptr<IArray> const &other) override
  {
    Ptr<Array<ElementType>> const &other_array    = other;
    auto const &                   other_elements = other_array->elements;

    elements.reserve(elements.size() + other_elements.size());

    elements.insert(elements.cend(), other_elements.cbegin(), other_elements.cend());
  }

  void Erase(int32_t const index) override
  {
    if (index < 0)
    {
      RuntimeError("negative index");
      return;
    }

    if (static_cast<std::size_t>(index) >= elements.size())
    {
      RuntimeError("index out of bounds");
      return;
    }

    elements.erase(elements.cbegin() + index);
  }

  TemplateParameter1 GetIndexedValue(AnyInteger const &index) override
  {
    ElementType *ptr = Find(index);
    if (ptr)
    {
      return TemplateParameter1(*ptr, element_type_id);
    }
    // Not found
    return TemplateParameter1();
  }

  void SetIndexedValue(AnyInteger const &index, TemplateParameter1 const &value) override
  {
    ElementType *ptr = Find(index);
    if (ptr)
    {
      *ptr = value.Get<ElementType>();
    }
  }

  ElementType *Find(Variant const &index)
  {
    std::size_t i;
    if (!GetNonNegativeInteger(index, i))
    {
      RuntimeError("negative index");
      return nullptr;
    }
    if (i >= elements.size())
    {
      RuntimeError("index out of bounds");
      return nullptr;
    }
    ElementType &element = elements[i];
    return &element;
  }

  bool SerializeTo(MsgPackSerializer &buffer) override
  {
    return ApplySerialize(buffer, elements);
  }

  bool DeserializeFrom(MsgPackSerializer &buffer) override
  {
    return ApplyDeserialize(buffer, elements);
  }

  TypeId element_type_id;
  // ElementType must NOT be bool because std::vector<bool> is a partial specialisation
  std::vector<ElementType> elements;

private:
  bool ApplySerialize(MsgPackSerializer &buffer, std::vector<Ptr<Object>> const &data)
  {
    if (!vm_->IsDefaultSerializeConstructable(element_type_id))
    {
      vm_->RuntimeError("Cannot serialize type " + vm_->GetTypeName(element_type_id) +
                        " as no serialisation constructor exists.");
      return false;
    }

    // Creating new array
    auto constructor  = buffer.NewArrayConstructor();
    auto array_buffer = constructor(elements.size());

    // Serializing elements
    for (Ptr<Object> const &v : data)
    {
      if (!v)
      {
        RuntimeError("Cannot serialise null reference element in " + GetTypeName());
        return false;
      }

      auto success = array_buffer.AppendUsingFunction(
          [&v](MsgPackSerializer &serializer) { return v->SerializeTo(serializer); });

      if (!success)
      {
        return false;
      }
    }

    return true;
  }

  template <typename G>
  std::enable_if_t<IsPrimitive<G>, bool> ApplySerialize(MsgPackSerializer &   buffer,
                                                        std::vector<G> const &data)
  {
    // Creating new array
    auto constructor  = buffer.NewArrayConstructor();
    auto array_buffer = constructor(elements.size());

    // Serializing elements
    for (G const &v : data)
    {
      array_buffer.Append(v);
    }

    return true;
  }

  bool ApplyDeserialize(MsgPackSerializer &buffer, std::vector<Ptr<Object>> &data)
  {
    if (!vm_->IsDefaultSerializeConstructable(element_type_id))
    {
      vm_->RuntimeError("Cannot deserialize type " + vm_->GetTypeName(element_type_id) +
                        " as no serialisation constructor exists.");
      return false;
    }

    auto array = buffer.NewArrayDeserializer();
    data.resize(array.size());

    for (Ptr<Object> &v : data)
    {
      v = vm_->DefaultSerializeConstruct(element_type_id);
      if (!v)
      {
        return false;
      }

      // Deserializing next elemtn
      auto success = array.GetNextValueUsingFunction(
          [&v](MsgPackSerializer &serializer) { return v->DeserializeFrom(serializer); });

      if (!success)
      {
        return false;
      }
    }
    return true;
  }

  template <typename G>
  std::enable_if_t<IsPrimitive<G>, bool> ApplyDeserialize(MsgPackSerializer &buffer,
                                                          std::vector<G> &   data)
  {
    auto array = buffer.NewArrayDeserializer();

    data.resize(array.size());
    for (G &v : data)
    {
      array.GetNextValue(v);
    }

    return true;
  }
};

template <typename... Args>
Ptr<IArray> IArray::Construct(VM *vm, TypeId type_id, Args &&... args)
{
  TypeInfo const &type_info       = vm->GetTypeInfo(type_id);
  TypeId const    element_type_id = type_info.template_parameter_type_ids[0];
  switch (element_type_id)
  {
  case TypeIds::Bool:
  {
    return Ptr<IArray>{
        new Array<uint8_t>(vm, type_id, element_type_id, std::forward<Args>(args)...)};
  }
  case TypeIds::Int8:
  {
    return Ptr<IArray>{
        new Array<int8_t>(vm, type_id, element_type_id, std::forward<Args>(args)...)};
  }
  case TypeIds::UInt8:
  {
    return Ptr<IArray>{
        new Array<uint8_t>(vm, type_id, element_type_id, std::forward<Args>(args)...)};
  }
  case TypeIds::Int16:
  {
    return Ptr<IArray>{
        new Array<int16_t>(vm, type_id, element_type_id, std::forward<Args>(args)...)};
  }
  case TypeIds::UInt16:
  {
    return Ptr<IArray>{
        new Array<uint16_t>(vm, type_id, element_type_id, std::forward<Args>(args)...)};
  }
  case TypeIds::Int32:
  {
    return Ptr<IArray>{
        new Array<int32_t>(vm, type_id, element_type_id, std::forward<Args>(args)...)};
  }
  case TypeIds::UInt32:
  {
    return Ptr<IArray>{
        new Array<uint32_t>(vm, type_id, element_type_id, std::forward<Args>(args)...)};
  }
  case TypeIds::Int64:
  {
    return Ptr<IArray>{
        new Array<int64_t>(vm, type_id, element_type_id, std::forward<Args>(args)...)};
  }
  case TypeIds::UInt64:
  {
    return Ptr<IArray>{
        new Array<uint64_t>(vm, type_id, element_type_id, std::forward<Args>(args)...)};
  }
  case TypeIds::Fixed32:
  {
    return Ptr<IArray>{
        new Array<fixed_point::fp32_t>(vm, type_id, element_type_id, std::forward<Args>(args)...)};
  }
  case TypeIds::Fixed64:
  {
    return Ptr<IArray>{
        new Array<fixed_point::fp64_t>(vm, type_id, element_type_id, std::forward<Args>(args)...)};
  }
  case TypeIds::Fixed128:
  {
    return Ptr<IArray>{
        new Array<Ptr<Fixed128>>(vm, type_id, element_type_id, std::forward<Args>(args)...)};
  }
  default:
  {
    return Ptr<IArray>{
        new Array<Ptr<Object>>(vm, type_id, element_type_id, std::forward<Args>(args)...)};
  }
  }  // switch
}

inline Ptr<IArray> IArray::Constructor(VM *vm, TypeId type_id, int32_t size)
{
  if (size < 0)
  {
    vm->RuntimeError("negative size");

    return {};
  }
  return Construct(vm, type_id, size);
}

}  // namespace vm
}  // namespace fetch
