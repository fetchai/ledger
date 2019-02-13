#pragma once
#include "byte_array_wrapper.hpp"
#include "core/byte_array/encoders.hpp"

namespace fetch
{
namespace modules
{

void PrintByteArray(fetch::vm::VM * /*vm*/, fetch::vm::Ptr<fetch::modules::ByteArrayWrapper> const &s)
{
//  std::cout << byte_array::ToBase64(s->byte_array());
  std::cout << s->byte_array();
}

void PrintLnByteArray(fetch::vm::VM * /*vm*/, fetch::vm::Ptr<fetch::modules::ByteArrayWrapper> const &s)
{
//  std::cout << byte_array::ToBase64(s->byte_array()) << std::endl;
  std::cout << s->byte_array() << std::endl;
}

void PrintString(fetch::vm::VM * /*vm*/, fetch::vm::Ptr<fetch::vm::String> const &s)
{
  std::cout << s->str;
}

void PrintLnString(fetch::vm::VM * /*vm*/, fetch::vm::Ptr<fetch::vm::String> const &s)
{
  std::cout << s->str << std::endl;
}


void PrintInt32(fetch::vm::VM * /*vm*/, int32_t const &s)
{
  std::cout << s;
}

void PrintLnInt32(fetch::vm::VM * /*vm*/, int32_t const &s)
{
  std::cout << s << std::endl;
}

void PrintInt64(fetch::vm::VM * /*vm*/, int64_t const &s)
{
  std::cout << s;
}

void PrintLnInt64(fetch::vm::VM * /*vm*/, int64_t const &s)
{
  std::cout << s << std::endl;
}

void BindPrint(vm::Module &module)
{
  module.CreateFreeFunction("Print", &PrintByteArray);
  module.CreateFreeFunction("PrintLn", &PrintLnByteArray);
  module.CreateFreeFunction("Print", &PrintString);
  module.CreateFreeFunction("PrintLn", &PrintLnString);
  module.CreateFreeFunction("Print", &PrintInt32);
  module.CreateFreeFunction("PrintLn", &PrintLnInt32);
  module.CreateFreeFunction("Print", &PrintInt64);
  module.CreateFreeFunction("PrintLn", &PrintLnInt64);
}


}
}