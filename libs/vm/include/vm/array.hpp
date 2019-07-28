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
#include "core/serializers/base_types.hpp"
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
struct GetElementType<T, typename std::enable_if_t<std::is_same<T, bool>::value>>
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

    return array;
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

    return array;
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

  void Erase(const int32_t index) override
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
      vm_->RuntimeError("Cannot deserialize type " + vm_->GetUniqueId(element_type_id) +
                        " as no serialisation constructor exists.");
      return false;
    }

    buffer << GetUniqueId() << static_cast<uint64_t>(elements.size());
    for (Ptr<Object> v : data)
    {
      if (!v)
      {
        RuntimeError("Cannot serialise null reference element in " + GetUniqueId());
        return false;
      }

      if (!v->SerializeTo(buffer))
      {
        return false;
      }
    }
    return true;
  }

  template <typename G>
  typename std::enable_if<IsPrimitive<G>::value, bool>::type ApplySerialize(
      MsgPackSerializer &buffer, std::vector<G> const &data)
  {
    buffer << GetUniqueId() << static_cast<uint64_t>(elements.size());
    for (G const &v : data)
    {
      buffer << v;
    }
    return true;
  }

  bool ApplyDeserialize(MsgPackSerializer &buffer, std::vector<Ptr<Object>> &data)
  {
    uint64_t    size;
    std::string uid;
    buffer >> uid >> size;

    if (uid != GetUniqueId())
    {
      vm_->RuntimeError("Type mismatch during deserialization. Got " + uid + " but expected " +
                        GetUniqueId());
      return false;
    }

    data.resize(size);

    if (!vm_->IsDefaultSerializeConstructable(element_type_id))
    {
      vm_->RuntimeError("Cannot deserialize type " + vm_->GetUniqueId(element_type_id) +
                        " as no serialisation constructor exists.");
      return false;
    }

    data.resize(size);
    for (Ptr<Object> &v : data)
    {
      v = vm_->DefaultSerializeConstruct(element_type_id);
      if (!v || !v->DeserializeFrom(buffer))
      {
        return false;
      }
    }
    return true;
  }

  template <typename G>
  typename std::enable_if<IsPrimitive<G>::value, bool>::type ApplyDeserialize(
      MsgPackSerializer &buffer, std::vector<G> &data)
  {
    uint64_t    size;
    std::string uid;
    buffer >> uid >> size;
    if (uid != GetUniqueId())
    {
      vm_->RuntimeError("Type mismatch during deserialization. Got " + uid + " but expected " +
                        GetUniqueId());
      return false;
    }

    data.resize(size);
    for (G &v : data)
    {
      buffer >> v;
    }
    return true;
  }
};

template <typename... Args>
inline Ptr<IArray> IArray::Construct(VM *vm, TypeId type_id, Args &&... args)
{
  TypeInfo const &type_info       = vm->GetTypeInfo(type_id);
  TypeId const    element_type_id = type_info.parameter_type_ids[0];
  switch (element_type_id)
  {
  case TypeIds::Bool:
  {
    return new Array<uint8_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int8:
  {
    return new Array<int8_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::UInt8:
  {
    return new Array<uint8_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int16:
  {
    return new Array<int16_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::UInt16:
  {
    return new Array<uint16_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int32:
  {
    return new Array<int32_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::UInt32:
  {
    return new Array<uint32_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Int64:
  {
    return new Array<int64_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::UInt64:
  {
    return new Array<uint64_t>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Float32:
  {
    return new Array<float>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Float64:
  {
    return new Array<double>(vm, type_id, element_type_id, std::forward<Args>(args)...);
  }
  case TypeIds::Fixed32:
  {
    return new Array<fixed_point::fp32_t>(vm, type_id, element_type_id,
                                          std::forward<Args>(args)...);
  }
  case TypeIds::Fixed64:
  {
    return new Array<fixed_point::fp64_t>(vm, type_id, element_type_id,
                                          std::forward<Args>(args)...);
  }
  default:
  {
    return new Array<Ptr<Object>>(vm, type_id, element_type_id, std::forward<Args>(args)...);
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
