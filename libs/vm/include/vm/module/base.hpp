#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

namespace fetch {
namespace vm {

template <typename T>
struct IsResult
{
  static const int value = 1;
};
template <>
struct IsResult<void>
{
  static const int value = 0;
};

template <typename T>
struct StackGetter
{
  static T Get(VM *vm, int sp_offset)
  {
    Variant & v = vm->stack_[vm->sp_ - sp_offset];
    return v.Move<T>();
  }
};

template <typename T>
struct StackSetter
{
  static void Set(VM *vm, int sp_offset, T && result, TypeId type_id)
  {
    Variant & v = vm->stack_[vm->sp_ - sp_offset];
    v.Assign(std::move(result), type_id);
  }
};

template <typename T, typename = void>
struct TypeGetter;
template <typename T>
struct TypeGetter<T, typename std::enable_if_t<
  is_primitive<T>::value ||
  is_variant<T>::value>>
{
  static TypeIndex GetTypeIndex()
  {
    return TypeIndex(typeid(T));
  }
};
template <typename T>
struct TypeGetter<T, typename std::enable_if_t<is_ptr<T>::value>>
{
  static TypeIndex GetTypeIndex()
  {
    using ManagedType = typename ptr_managed_type<T>::type;
    return TypeIndex(typeid(ManagedType));
  }
};

template <typename T, typename = void>
struct ParameterTypeGetter;
template <typename T>
struct ParameterTypeGetter<T, typename std::enable_if_t<
  is_primitive_parameter<T>::value ||
  is_variant_parameter<T>::value>>
{
  static TypeIndex GetTypeIndex()
  {
    return TypeIndex(typeid(std::decay_t<T>));
  }
};
template <typename T>
struct ParameterTypeGetter<T, typename std::enable_if_t<is_ptr_parameter<T>::value>>
{
  static TypeIndex GetTypeIndex()
  {
    using ManagedType = typename ptr_managed_type<std::decay_t<T>>::type;
    return TypeIndex(typeid(ManagedType));
  }
};

template <typename ...Ts>
struct UnrollTypes;
template <typename T, typename ...Ts>
struct UnrollTypes<T, Ts...>
{
  // Invoked on non-final type
  static void Unroll(TypeIndexArray & array)
  {
    TypeIndex const type_index = TypeGetter<T>::GetTypeIndex();
    array.push_back(type_index);
    UnrollTypes<Ts...>::Unroll(array);
  }
};
template <typename T>
struct UnrollTypes<T>
{
  // Invoked on final type
  static void Unroll(TypeIndexArray & array)
  {
    TypeIndex const type_index = TypeGetter<T>::GetTypeIndex();
    array.push_back(type_index);
  }
};
template <>
struct UnrollTypes<>
{
  // Invoked on zero types
  static void Unroll(TypeIndexArray & /* array */)
  {}
};

template <typename ...Ts>
struct UnrollParameterTypes;
template <typename T, typename ...Ts>
struct UnrollParameterTypes<T, Ts...>
{
  // Invoked on non-final type
  static void Unroll(TypeIndexArray & array)
  {
    TypeIndex const type_index = ParameterTypeGetter<T>::GetTypeIndex();
    array.push_back(type_index);
    UnrollParameterTypes<Ts...>::Unroll(array);
  }
};
template <typename T>
struct UnrollParameterTypes<T>
{
  // Invoked on final type
  static void Unroll(TypeIndexArray & array)
  {
    TypeIndex const type_index = ParameterTypeGetter<T>::GetTypeIndex();
    array.push_back(type_index);
  }
};
template <>
struct UnrollParameterTypes<>
{
  // Invoked on zero types
  static void Unroll(TypeIndexArray & /* array */)
  {}
};

} // namespace vm
} // namespace fetch
