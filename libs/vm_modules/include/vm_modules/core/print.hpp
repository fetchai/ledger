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

/**
 * method for printing string to std::cout
 */

/*
inline void PrintByteArray(fetch::vm::VM *                                            vm,
                         fetch::vm::Ptr<fetch::vm_modules::ByteArrayWrapper> const &s)
{
auto &out = vm->GetOutputDevice("stdout");
out << s->byte_array();
}

inline void PrintLnByteArray(fetch::vm::VM *                                            vm,
                           fetch::vm::Ptr<fetch::vm_modules::ByteArrayWrapper> const &s)
{
auto &out = vm->GetOutputDevice("stdout");
out << byte_array::ToHex(s->byte_array()) << std::endl;
}
*/

inline void PrintString(fetch::vm::VM * /*vm*/, fetch::vm::Ptr<fetch::vm::String> const &s)
{
  std::cout << s->str;
}

inline void PrintLnString(fetch::vm::VM * /*vm*/, fetch::vm::Ptr<fetch::vm::String> const &s)
{
  std::cout << s->str << std::endl;
}

inline void PrintInt32(fetch::vm::VM * /*vm*/, int32_t const &s)
{
  std::cout << s;
}

inline void PrintLnInt32(fetch::vm::VM * /*vm*/, int32_t const &s)
{

  std::cout << s << std::endl;
}

inline void PrintInt64(fetch::vm::VM * /*vm*/, int64_t const &s)
{

  std::cout << s;
}

inline void PrintLnInt64(fetch::vm::VM * /*vm*/, int64_t const &s)
{

  std::cout << s << std::endl;
}

inline void PrintFloat(fetch::vm::VM * /*vm*/, float const &s)
{

  std::cout << s;
}

inline void PrintLnFloat(fetch::vm::VM * /*vm*/, float const &s)
{

  std::cout << s;
}

inline void PrintDouble(fetch::vm::VM * /*vm*/, double const &s)
{

  std::cout << s;
}

inline void PrintLnDouble(fetch::vm::VM * /*vm*/, double const &s)
{

  std::cout << s << std::endl;
}

template <typename T, bool NL = false>
inline void PrintArrayPrimitive(fetch::vm::VM * /*vm*/, vm::Ptr<vm::Array<T>> const &g)
{

  std::cout << "[";
  for (std::size_t i = 0; i < g->elements.size(); ++i)
  {
    if (i != 0)
    {
      std::cout << ", ";
    }
    std::cout << g->elements[i];
  }
  std::cout << "]";
  if (NL)
  {
    std::cout << std::endl;
  }
}

inline void CreatePrint(vm::Module &module)
{
  /*
  module.CreateFreeFunction("print", &PrintByteArray);
  module.CreateFreeFunction("printLn", &PrintLnByteArray);
  */

  module.CreateFreeFunction("print", &PrintString);
  module.CreateFreeFunction("printLn", &PrintLnString);
  module.CreateFreeFunction("print", &PrintInt32);
  module.CreateFreeFunction("printLn", &PrintLnInt32);
  module.CreateFreeFunction("print", &PrintInt64);
  module.CreateFreeFunction("printLn", &PrintLnInt64);
  module.CreateFreeFunction("print", &PrintDouble);
  module.CreateFreeFunction("printLn", &PrintLnDouble);
  module.CreateFreeFunction("print", &PrintFloat);
  module.CreateFreeFunction("printLn", &PrintLnFloat);

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
