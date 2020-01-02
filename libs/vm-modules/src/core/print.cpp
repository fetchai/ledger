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

#include "meta/type_traits.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/array.hpp"
#include "vm/fixed.hpp"
#include "vm/module.hpp"
#include "vm/object.hpp"
#include "vm/vm.hpp"
#include "vm_modules/core/print.hpp"

#include <ostream>

using namespace fetch::vm;

namespace fetch {
namespace vm_modules {

namespace {
namespace internal {

using meta::EnableIf;
using meta::IsAny8BitInteger;
using meta::IsNotAny8BitInteger;

template <bool APPEND_LINEBREAK>
void FlushOutput(std::ostream &out)
{
  if (APPEND_LINEBREAK)
  {
    out << std::endl;
  }
  else
  {
    out << std::flush;
  }
}

void StringifyNullPtr(std::ostream &out)
{
  out << "(nullptr)";
}

void StringifyBool(std::ostream &out, bool b)
{
  out << (b ? "true" : "false");
}

template <typename T>
void StringifyLargeNumber(std::ostream &out, T const &el)
{
  if (el == nullptr)
  {
    internal::StringifyNullPtr(out);
  }
  else
  {
    out << reinterpret_cast<Ptr<Fixed128> const &>(el)->data_;
  }
}

template <typename T>
inline EnableIf<IsNotAny8BitInteger<T>> StringifyNumber(std::ostream &out, T const &el)
{
  out << el;
}

// int8_t and uint8_t need casting to int32_t, or they get mangled to ASCII characters
template <typename T>
inline EnableIf<IsAny8BitInteger<T>> StringifyNumber(std::ostream &out, T const &el)
{
  out << static_cast<int32_t>(el);
}

template <typename T>
void StringifyArrayElement(TypeId type_id, std::ostream &out, T const &el)
{
  if (type_id == TypeIds::Bool)
  {
    StringifyBool(out, static_cast<bool>(el));
  }
  else
  {
    StringifyNumber(out, el);
  }
}

template <typename T>
void StringifyArrayLargeElement(TypeId type_id, std::ostream &out, T const &el)
{
  if (type_id == TypeIds::Bool)
  {
    StringifyBool(out, static_cast<bool>(el));
  }
  else
  {
    StringifyLargeNumber(out, el);
  }
}

}  // namespace internal

template <bool APPEND_LINEBREAK = false>
void PrintString(VM *vm, Ptr<String> const &s)
{
  auto &out = vm->GetOutputDevice(VM::STDOUT);

  if (s == nullptr)
  {
    internal::StringifyNullPtr(out);
  }
  else
  {
    out << s->string();
  }

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

template <typename T, bool APPEND_LINEBREAK = false>
void PrintNumber(VM *vm, T const &s)
{
  auto &out = vm->GetOutputDevice(VM::STDOUT);

  internal::StringifyNumber(out, s);

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

template <typename T, bool APPEND_LINEBREAK = false>
void PrintLargeNumber(VM *vm, T const &s)
{
  auto &out = vm->GetOutputDevice(VM::STDOUT);

  internal::StringifyLargeNumber(out, s);

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

template <bool APPEND_LINEBREAK = false>
void PrintBool(VM *vm, bool const &s)
{
  auto &out = vm->GetOutputDevice(VM::STDOUT);

  internal::StringifyBool(out, s);

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

template <typename T, bool APPEND_LINEBREAK = false>
void PrintArray(VM *vm, Ptr<Array<T>> const &arr)
{
  auto &out = vm->GetOutputDevice(VM::STDOUT);

  if (arr == nullptr)
  {
    internal::StringifyNullPtr(out);
  }
  else
  {
    out << "[";
    if (!arr->elements.empty())
    {
      internal::StringifyArrayElement(arr->element_type_id, out, arr->elements[0]);
      for (std::size_t i = 1; i < arr->elements.size(); ++i)
      {
        out << ", ";
        internal::StringifyArrayElement(arr->element_type_id, out, arr->elements[i]);
      }
    }
    out << "]";
  }

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

template <typename T, bool APPEND_LINEBREAK = false>
void PrintLargeArray(VM *vm, Ptr<Array<T>> const &arr)
{
  auto &out = vm->GetOutputDevice(VM::STDOUT);

  if (arr == nullptr)
  {
    internal::StringifyNullPtr(out);
  }
  else
  {
    out << "[";
    if (!arr->elements.empty())
    {
      internal::StringifyArrayLargeElement(arr->element_type_id, out, arr->elements[0]);
      for (std::size_t i = 1; i < arr->elements.size(); ++i)
      {
        out << ", ";
        internal::StringifyArrayLargeElement(arr->element_type_id, out, arr->elements[i]);
      }
    }
    out << "]";
  }

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

}  // namespace

void CreatePrint(Module &module)
{
  module.CreateFreeFunction("print", &PrintString<>);
  module.CreateFreeFunction("printLn", &PrintString<true>);

  module.CreateFreeFunction("print", &PrintBool<>);
  module.CreateFreeFunction("printLn", &PrintBool<true>);

  module.CreateFreeFunction("print", &PrintNumber<uint8_t>);
  module.CreateFreeFunction("printLn", &PrintNumber<uint8_t, true>);
  module.CreateFreeFunction("print", &PrintNumber<int8_t>);
  module.CreateFreeFunction("printLn", &PrintNumber<int8_t, true>);

  module.CreateFreeFunction("print", &PrintNumber<uint16_t>);
  module.CreateFreeFunction("printLn", &PrintNumber<uint16_t, true>);
  module.CreateFreeFunction("print", &PrintNumber<int16_t>);
  module.CreateFreeFunction("printLn", &PrintNumber<int16_t, true>);

  module.CreateFreeFunction("print", &PrintNumber<uint32_t>);
  module.CreateFreeFunction("printLn", &PrintNumber<uint32_t, true>);
  module.CreateFreeFunction("print", &PrintNumber<int32_t>);
  module.CreateFreeFunction("printLn", &PrintNumber<int32_t, true>);

  module.CreateFreeFunction("print", &PrintNumber<uint64_t>);
  module.CreateFreeFunction("printLn", &PrintNumber<uint64_t, true>);
  module.CreateFreeFunction("print", &PrintNumber<int64_t>);
  module.CreateFreeFunction("printLn", &PrintNumber<int64_t, true>);

  module.CreateFreeFunction("print", &PrintNumber<fixed_point::fp32_t>);
  module.CreateFreeFunction("print", &PrintNumber<fixed_point::fp64_t>);
  module.CreateFreeFunction("print", &PrintLargeNumber<Ptr<Fixed128>>);
  module.CreateFreeFunction("printLn", &PrintNumber<fixed_point::fp32_t, true>);
  module.CreateFreeFunction("printLn", &PrintNumber<fixed_point::fp64_t, true>);
  module.CreateFreeFunction("printLn", &PrintLargeNumber<Ptr<Fixed128>, true>);

  module.CreateFreeFunction("print", &PrintArray<bool>);
  module.CreateFreeFunction("printLn", &PrintArray<bool, true>);

  module.CreateFreeFunction("print", &PrintArray<uint8_t>);
  module.CreateFreeFunction("printLn", &PrintArray<uint8_t, true>);
  module.CreateFreeFunction("print", &PrintArray<int8_t>);
  module.CreateFreeFunction("printLn", &PrintArray<int8_t, true>);

  module.CreateFreeFunction("print", &PrintArray<uint16_t>);
  module.CreateFreeFunction("printLn", &PrintArray<uint16_t, true>);
  module.CreateFreeFunction("print", &PrintArray<int16_t>);
  module.CreateFreeFunction("printLn", &PrintArray<int16_t, true>);

  module.CreateFreeFunction("print", &PrintArray<uint32_t>);
  module.CreateFreeFunction("printLn", &PrintArray<uint32_t, true>);
  module.CreateFreeFunction("print", &PrintArray<int32_t>);
  module.CreateFreeFunction("printLn", &PrintArray<int32_t, true>);

  module.CreateFreeFunction("print", &PrintArray<uint64_t>);
  module.CreateFreeFunction("printLn", &PrintArray<uint64_t, true>);
  module.CreateFreeFunction("print", &PrintArray<int64_t>);
  module.CreateFreeFunction("printLn", &PrintArray<int64_t, true>);

  module.CreateFreeFunction("print", &PrintArray<fixed_point::fp32_t>);
  module.CreateFreeFunction("print", &PrintArray<fixed_point::fp64_t>);
  module.CreateFreeFunction("print", &PrintLargeArray<Ptr<Fixed128>>);
  module.CreateFreeFunction("printLn", &PrintArray<fixed_point::fp32_t, true>);
  module.CreateFreeFunction("printLn", &PrintArray<fixed_point::fp64_t, true>);
  module.CreateFreeFunction("printLn", &PrintLargeArray<Ptr<Fixed128>, true>);
}

}  // namespace vm_modules
}  // namespace fetch
