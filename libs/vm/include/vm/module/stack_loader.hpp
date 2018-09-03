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

namespace details {

template <typename T>
struct HasResult
{
  enum
  {
    value = 1
  };
};

template <>
struct HasResult<void>
{
  enum
  {
    value = 0
  };
};

template <int N>
struct Resetter
{
  static void Reset(VM *vm)
  {
    Value &element = vm->stack_[vm->sp_ - N + 1];
    element.Reset();
    Resetter<N - 1>::Reset(vm);
  }
};

template <>
struct Resetter<0>
{
  static void Reset(VM *vm) {}
};

// In case we are adding to the stack there is nothing to reset
template <>
struct Resetter<-1>
{
  static void Reset(VM *vm) {}
};

/* 
 * Storing and loading of objects.
 *
 * The default implementation wraps any type of C++ class. Subsequent code 
 * specialises for builtin types both primitive and advanced types.
 */
template <typename T, int N>
struct StorerClass
{

  static void StoreArgument(VM *vm, T &&val)
  {
    WrapperClass<T> *obj   = new WrapperClass<T>(vm->instruction_->type_id, vm, std::move(val));
    Value &          value = vm->stack_[vm->sp_ - N];

    value.Reset();
    value.SetObject(obj, vm->instruction_->type_id);
  }
};

template <typename T>
struct LoaderClass
{

  static T LoadArgument(int const &N, VM *vm)
  {
    Value &          element = vm->stack_[vm->sp_ - N];
    WrapperClass<T> *p       = static_cast<WrapperClass<T> *>(element.variant.object);

    return p->object;
  }
};

/* 
 * Storing of primitive builtins.
 */
template <int N>
struct StorerClass<bool, N>
{

  static void StoreArgument(VM *vm, bool &&val)
  {
    assert(N <= vm->sp_);
    assert(vm->instruction_->type_id == TypeId::Bool);

    Value &value = vm->stack_[vm->sp_ - N];

    value.Reset();
    value.variant.ui8 = val ? uint8_t(-1) : 0;
    value.type_id     = vm->instruction_->type_id;
  }
};

template <int N>
struct StorerClass<uint8_t, N>
{

  static void StoreArgument(VM *vm, uint8_t &&val)
  {
    assert(N <= vm->sp_);
    assert(vm->instruction_->type_id == TypeId::Byte);

    Value &value = vm->stack_[vm->sp_ - N];

    value.Reset();
    value.variant.ui8 = val;
    value.type_id     = vm->instruction_->type_id;
  }
};

template <int N>
struct StorerClass<int8_t, N>
{

  static void StoreArgument(VM *vm, int8_t &&val)
  {
    assert(N <= vm->sp_);
    assert(vm->instruction_->type_id == TypeId::Int8);

    Value &value = vm->stack_[vm->sp_ - N];

    value.Reset();
    value.variant.i8 = val;
    value.type_id     = vm->instruction_->type_id;
  }
};


template <int N>
struct StorerClass<uint16_t, N>
{

  static void StoreArgument(VM *vm, uint16_t &&val)
  {
    assert(N <= vm->sp_);
    assert(vm->instruction_->type_id == TypeId::UInt16);

    Value &value = vm->stack_[vm->sp_ - N];

    value.Reset();
    value.variant.ui16 = val;
    value.type_id     = vm->instruction_->type_id;
  }
};

template <int N>
struct StorerClass<int16_t, N>
{

  static void StoreArgument(VM *vm, int16_t &&val)
  {
    assert(N <= vm->sp_);
    assert(vm->instruction_->type_id == TypeId::Int16);

    Value &value = vm->stack_[vm->sp_ - N];

    value.Reset();
    value.variant.i16 = val;
    value.type_id     = vm->instruction_->type_id;
  }
};



template <int N>
struct StorerClass<uint32_t, N>
{

  static void StoreArgument(VM *vm, uint32_t &&val)
  {
    assert(N <= vm->sp_);
    assert(vm->instruction_->type_id == TypeId::UInt32);

    Value &value = vm->stack_[vm->sp_ - N];

    value.Reset();
    value.variant.ui32 = val;
    value.type_id     = vm->instruction_->type_id;
  }
};

template <int N>
struct StorerClass<int32_t, N>
{

  static void StoreArgument(VM *vm, int32_t &&val)
  {
    assert(N <= vm->sp_);
    assert(vm->instruction_->type_id == TypeId::Int32);

    Value &value = vm->stack_[vm->sp_ - N];

    value.Reset();
    value.variant.i32 = val;
    value.type_id     = vm->instruction_->type_id;
  }
};



template <int N>
struct StorerClass<uint64_t, N>
{

  static void StoreArgument(VM *vm, uint64_t &&val)
  {
    assert(N <= vm->sp_);
    assert(vm->instruction_->type_id == TypeId::UInt64);

    Value &value = vm->stack_[vm->sp_ - N];

    value.Reset();
    value.variant.ui64 = val;
    value.type_id     = vm->instruction_->type_id;
  }
};

template <int N>
struct StorerClass<int64_t, N>
{

  static void StoreArgument(VM *vm, int64_t &&val)
  {
    assert(N <= vm->sp_);
    assert(vm->instruction_->type_id == TypeId::Int64);

    Value &value = vm->stack_[vm->sp_ - N];

    value.Reset();
    value.variant.i64 = val;
    value.type_id     = vm->instruction_->type_id;
  }
};


template <int N>
struct StorerClass<float, N>
{

  static void StoreArgument(VM *vm, float &&val)
  {
    assert(N <= vm->sp_);
    assert(vm->instruction_->type_id == TypeId::Float32);

    Value &value = vm->stack_[vm->sp_ - N];

    value.Reset();
    value.variant.f32 = val;
    value.type_id     = vm->instruction_->type_id;
  }
};


template <int N>
struct StorerClass<double, N>
{

  static void StoreArgument(VM *vm, double &&val)
  {
    assert(N <= vm->sp_);
    assert(vm->instruction_->type_id == TypeId::Float64);

    Value &value = vm->stack_[vm->sp_ - N];

    value.Reset();
    value.variant.f64 = val;
    value.type_id     = vm->instruction_->type_id;
  }
};


/* 
 * Storing of advanced builtin objects
 */
template <int N>
struct StorerClass<fetch::math::linalg::Matrix<double, fetch::memory::Array<double>>, N>
{

  static void StoreArgument(VM *                                                                vm,
                            fetch::math::linalg::Matrix<double, fetch::memory::Array<double>> &&val)
  {
    assert(N <= vm->sp_);
    assert(vm->instruction_->type_id == TypeId::Matrix_Float64);

    MatrixFloat64 *m = new MatrixFloat64(TypeId::Matrix_Float64, vm, std::move(val));

    Value &value = vm->stack_[vm->sp_ - N];
    value.Reset();
    value.SetObject(m, vm->instruction_->type_id);
  }
};


/* 
 * Loading of primitive builtins.
 */
template <>
struct LoaderClass<int8_t>
{
  static int8_t LoadArgument(int const &N, VM *vm)
  {
    Value &element = vm->stack_[vm->sp_ - N];
    return element.variant.i8;
  }
};

template <>
struct LoaderClass<int16_t>
{
  static int16_t LoadArgument(int const &N, VM *vm)
  {
    Value &element = vm->stack_[vm->sp_ - N];
    return element.variant.i16;
  }
};

template <>
struct LoaderClass<int32_t>
{
  static int32_t LoadArgument(int const &N, VM *vm)
  {
    Value &element = vm->stack_[vm->sp_ - N];
    return element.variant.i32;
  }
};

template <>
struct LoaderClass<int64_t>
{
  static int64_t LoadArgument(int const &N, VM *vm)
  {
    Value &element = vm->stack_[vm->sp_ - N];
    return element.variant.i64;
  }
};

template <>
struct LoaderClass<uint8_t>
{
  static uint8_t LoadArgument(int const &N, VM *vm)
  {
    Value &element = vm->stack_[vm->sp_ - N];
    return element.variant.ui8;
  }
};

template <>
struct LoaderClass<uint16_t>
{
  static uint16_t LoadArgument(int const &N, VM *vm)
  {
    Value &element = vm->stack_[vm->sp_ - N];
    return element.variant.ui16;
  }
};

template <>
struct LoaderClass<uint32_t>
{
  static uint32_t LoadArgument(int const &N, VM *vm)
  {
    Value &element = vm->stack_[vm->sp_ - N];
    return element.variant.ui32;
  }
};

template <>
struct LoaderClass<uint64_t>
{
  static uint64_t LoadArgument(int const &N, VM *vm)
  {
    Value &element = vm->stack_[vm->sp_ - N];
    return element.variant.ui64;
  }
};

template <>
struct LoaderClass<double>
{
  static double LoadArgument(int const &N, VM *vm)
  {
    Value &element = vm->stack_[vm->sp_ - N];
    return element.variant.f64;
  }
};

template <>
struct LoaderClass<float>
{
  static float LoadArgument(int const &N, VM *vm)
  {
    Value &element = vm->stack_[vm->sp_ - N];
    return element.variant.f32;
  }
};


/* 
 * Loading of advanced builtins.
 */
template <>
struct LoaderClass<fetch::math::linalg::Matrix<double, fetch::memory::Array<double>>>
{

  static fetch::math::linalg::Matrix<double, fetch::memory::Array<double>> LoadArgument(
      int const &N, VM *vm)
  {

    Value &        element = vm->stack_[vm->sp_ - N];
    MatrixFloat64 *p       = static_cast<MatrixFloat64 *>(element.variant.object);
    return p->matrix;
  }
};


}  // namespace details
}  // namespace vm
}  // namespace fetch
