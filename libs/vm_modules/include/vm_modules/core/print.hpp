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

#include "byte_array_wrapper.hpp"
#include "core/byte_array/encoders.hpp"

#include <ostream>

namespace fetch {
namespace vm_modules {

namespace internal {

template <bool APPEND_LINEBREAK>
inline void FlushOutput(std::ostream &out)
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

template <bool APPEND_LINEBREAK>
inline void PrintNullPtr(std::ostream &out)
{
  out << "(nullptr)";
  FlushOutput<APPEND_LINEBREAK>(out);
}

}  // namespace internal

template <bool APPEND_LINEBREAK = false>
inline void PrintString(fetch::vm::VM *vm, fetch::vm::Ptr<fetch::vm::String> const &s)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);
  out << s->str;

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

template <typename T, bool APPEND_LINEBREAK = false>
inline void PrintNumber(fetch::vm::VM *vm, T const &s)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);
  out << s;

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

template <typename T, bool APPEND_LINEBREAK = false>
inline void PrintByte(fetch::vm::VM *vm, T const &s)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);
  out << static_cast<int32_t>(s);

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

template <bool APPEND_LINEBREAK = false>
inline void PrintBool(fetch::vm::VM *vm, bool const &s)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);

  out << (s ? "true" : "false");

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

template <typename T, bool APPEND_LINEBREAK = false>
inline void PrintArrayPrimitiveByte(fetch::vm::VM *vm, vm::Ptr<vm::Array<T>> const &g)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);
  if (g == nullptr)
  {
    internal::PrintNullPtr<APPEND_LINEBREAK>(out);
    return;
  }

  assert(g != nullptr);

  out << "[";
  if (!g->elements.empty())
  {
    out << static_cast<int32_t>(g->elements[0]);
    for (std::size_t i = 1; i < g->elements.size(); ++i)
    {
      out << ", " << static_cast<int32_t>(g->elements[i]);
    }
  }
  out << "]";

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

template <typename T, bool APPEND_LINEBREAK = false>
inline void PrintArrayPrimitive(fetch::vm::VM *vm, vm::Ptr<vm::Array<T>> const &g)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);
  if (g == nullptr)
  {
    internal::PrintNullPtr<APPEND_LINEBREAK>(out);
    return;
  }

  assert(g != nullptr);

  out << "[";
  if (!g->elements.empty())
  {
    out << g->elements[0];
    for (std::size_t i = 1; i < g->elements.size(); ++i)
    {
      out << ", " << g->elements[i];
    }
  }
  out << "]";

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

inline void CreatePrint(vm::Module &module)
{
  module.CreateFreeFunction("print", &PrintString<>);
  module.CreateFreeFunction("printLn", &PrintString<true>);

  module.CreateFreeFunction("print", &PrintBool<>);
  module.CreateFreeFunction("printLn", &PrintBool<true>);

  module.CreateFreeFunction("print", &PrintByte<uint8_t>);
  module.CreateFreeFunction("printLn", &PrintByte<uint8_t, true>);
  module.CreateFreeFunction("print", &PrintByte<int8_t>);
  module.CreateFreeFunction("printLn", &PrintByte<int8_t, true>);

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

  module.CreateFreeFunction("print", &PrintNumber<float>);
  module.CreateFreeFunction("printLn", &PrintNumber<float, true>);
  module.CreateFreeFunction("print", &PrintNumber<double>);
  module.CreateFreeFunction("printLn", &PrintNumber<double, true>);

  module.CreateFreeFunction("print", &PrintArrayPrimitiveByte<uint8_t>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitiveByte<uint8_t, true>);
  module.CreateFreeFunction("print", &PrintArrayPrimitiveByte<int8_t>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitiveByte<int8_t, true>);

  module.CreateFreeFunction("print", &PrintArrayPrimitive<uint16_t>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitive<uint16_t, true>);
  module.CreateFreeFunction("print", &PrintArrayPrimitive<int16_t>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitive<int16_t, true>);

  module.CreateFreeFunction("print", &PrintArrayPrimitive<uint32_t>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitive<uint32_t, true>);
  module.CreateFreeFunction("print", &PrintArrayPrimitive<int32_t>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitive<int32_t, true>);

  module.CreateFreeFunction("print", &PrintArrayPrimitive<uint64_t>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitive<uint64_t, true>);
  module.CreateFreeFunction("print", &PrintArrayPrimitive<int64_t>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitive<int64_t, true>);

  module.CreateFreeFunction("print", &PrintArrayPrimitive<float>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitive<float, true>);
  module.CreateFreeFunction("print", &PrintArrayPrimitive<double>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitive<double, true>);
}

inline void CreatePrint(std::shared_ptr<vm::Module> module)
{
  CreatePrint(*module.get());
}

}  // namespace vm_modules
}  // namespace fetch
