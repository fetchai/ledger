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

#include "vm_modules/core/print.hpp"

#include "meta/type_traits.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

#include <ostream>

namespace fetch {
namespace vm_modules {

namespace {
namespace internal {

using meta::EnableIf;
using meta::IsAny8BitInteger;

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

inline void PrintNullPtr(std::ostream &out)
{
  out << "(nullptr)";
}

template <typename T>
inline EnableIf<!IsAny8BitInteger<T>, void> PrintWithCastIf8Bit(std::ostream &out, T const &el)
{
  out << el;
}

// int8_t and uint8_t need casting to int32_t, or they get mangled to ASCII characters
template <typename T>
inline EnableIf<IsAny8BitInteger<T>, void> PrintWithCastIf8Bit(std::ostream &out, T const &el)
{
  out << static_cast<int32_t>(el);
}

}  // namespace internal

template <bool APPEND_LINEBREAK = false>
void PrintString(fetch::vm::VM *vm, fetch::vm::Ptr<fetch::vm::String> const &s)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);

  out << s->str;

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

template <typename T, bool APPEND_LINEBREAK = false>
void PrintNumber(fetch::vm::VM *vm, T const &s)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);

  internal::PrintWithCastIf8Bit(out, s);

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

template <bool APPEND_LINEBREAK = false>
void PrintBool(fetch::vm::VM *vm, bool const &s)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);

  out << (s ? "true" : "false");

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

template <typename T, bool APPEND_LINEBREAK = false>
void PrintArray(fetch::vm::VM *vm, vm::Ptr<vm::Array<T>> const &g)
{
  auto &out = vm->GetOutputDevice(vm::VM::STDOUT);

  if (g == nullptr)
  {
    internal::PrintNullPtr(out);
  }
  else
  {
    out << "[";
    if (!g->elements.empty())
    {
      internal::PrintWithCastIf8Bit(out, g->elements[0]);
      for (std::size_t i = 1; i < g->elements.size(); ++i)
      {
        out << ", ";
        internal::PrintWithCastIf8Bit(out, g->elements[i]);
      }
    }
    out << "]";
  }

  internal::FlushOutput<APPEND_LINEBREAK>(out);
}

}  // namespace

void CreatePrint(vm::Module &module)
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

  module.CreateFreeFunction("print", &PrintNumber<float>);
  module.CreateFreeFunction("printLn", &PrintNumber<float, true>);
  module.CreateFreeFunction("print", &PrintNumber<double>);
  module.CreateFreeFunction("printLn", &PrintNumber<double, true>);

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

  module.CreateFreeFunction("print", &PrintArray<float>);
  module.CreateFreeFunction("printLn", &PrintArray<float, true>);
  module.CreateFreeFunction("print", &PrintArray<double>);
  module.CreateFreeFunction("printLn", &PrintArray<double, true>);
}

}  // namespace vm_modules
}  // namespace fetch
