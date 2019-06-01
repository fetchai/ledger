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

namespace fetch {
namespace vm_modules {

inline void PrintString(fetch::vm::VM *vm, fetch::vm::Ptr<fetch::vm::String> const &s)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);
  out << s->str;
}

inline void PrintLnString(fetch::vm::VM *vm, fetch::vm::Ptr<fetch::vm::String> const &s)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);
  out << s->str << std::endl;
}
template <typename T>
inline void Print(fetch::vm::VM *vm, T const &s)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);
  out << s;
}

template <typename T>
inline void PrintLn(fetch::vm::VM *vm, T const &s)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);
  out << s << std::endl;
}

inline void PrintByte(fetch::vm::VM *vm, int8_t const &s)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);
  out << static_cast<int32_t>(s);
}

inline void PrintByteLn(fetch::vm::VM *vm, int8_t const &s)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);
  out << static_cast<int32_t>(s) << std::endl;
}

inline void PrintUnsignedByte(fetch::vm::VM *vm, uint8_t const &s)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);
  out << static_cast<uint32_t>(s);
}

inline void PrintUnsignedByteLn(fetch::vm::VM *vm, uint8_t const &s)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);
  out << static_cast<uint32_t>(s) << std::endl;
}

template <typename T, bool NL = false>
inline void PrintArrayPrimitive(fetch::vm::VM *vm, vm::Ptr<vm::Array<T>> const &g)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);
  if (g == nullptr)
  {
    out << "(nullptr)" << std::endl;
    return;
  }

  assert(g != nullptr);

  out << "[";
  for (std::size_t i = 0; i < g->elements.size(); ++i)
  {
    if (i != 0)
    {
      out << ", ";
    }
    out << g->elements[i];
  }
  out << "]";
  if (NL)
  {
    out << std::endl;
  }
}

inline void CreatePrint(vm::Module &module)
{
  module.CreateFreeFunction("print", &PrintString);
  module.CreateFreeFunction("printLn", &PrintLnString);

  module.CreateFreeFunction("print", &PrintUnsignedByte);
  module.CreateFreeFunction("printLn", &PrintUnsignedByteLn);
  module.CreateFreeFunction("print", &PrintByte);
  module.CreateFreeFunction("printLn", &PrintByteLn);

  module.CreateFreeFunction("print", &Print<uint16_t>);
  module.CreateFreeFunction("printLn", &PrintLn<uint16_t>);
  module.CreateFreeFunction("print", &Print<int16_t>);
  module.CreateFreeFunction("printLn", &PrintLn<int16_t>);

  module.CreateFreeFunction("print", &Print<uint32_t>);
  module.CreateFreeFunction("printLn", &PrintLn<uint32_t>);
  module.CreateFreeFunction("print", &Print<int32_t>);
  module.CreateFreeFunction("printLn", &PrintLn<int32_t>);

  module.CreateFreeFunction("print", &Print<uint64_t>);
  module.CreateFreeFunction("printLn", &PrintLn<uint64_t>);
  module.CreateFreeFunction("print", &Print<int64_t>);
  module.CreateFreeFunction("printLn", &PrintLn<int64_t>);

  module.CreateFreeFunction("print", &Print<float>);
  module.CreateFreeFunction("printLn", &PrintLn<float>);
  module.CreateFreeFunction("print", &Print<double>);
  module.CreateFreeFunction("printLn", &PrintLn<double>);

  module.CreateFreeFunction("print", &PrintArrayPrimitive<int32_t>);
  module.CreateFreeFunction("print", &PrintArrayPrimitive<int64_t>);
  module.CreateFreeFunction("print", &PrintArrayPrimitive<uint32_t>);
  module.CreateFreeFunction("print", &PrintArrayPrimitive<uint64_t>);
  module.CreateFreeFunction("print", &PrintArrayPrimitive<float>);
  module.CreateFreeFunction("print", &PrintArrayPrimitive<double>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitive<int32_t, true>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitive<int64_t, true>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitive<uint32_t, true>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitive<uint64_t, true>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitive<float, true>);
  module.CreateFreeFunction("printLn", &PrintArrayPrimitive<double, true>);
}

inline void CreatePrint(std::shared_ptr<vm::Module> module)
{
  CreatePrint(*module.get());
}

}  // namespace vm_modules
}  // namespace fetch
