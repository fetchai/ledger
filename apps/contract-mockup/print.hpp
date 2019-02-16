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

void PrintFloat(fetch::vm::VM * /*vm*/, float const &s)
{
  std::cout << s;
}

void PrintLnFloat(fetch::vm::VM * /*vm*/, float const &s)
{
  std::cout << s;
}

void PrintDouble(fetch::vm::VM * /*vm*/, double const &s)
{
  std::cout << s;
}

void PrintLnDouble(fetch::vm::VM * /*vm*/, double const &s)
{
  std::cout << s << std::endl;
}


template<typename T>
void PrintArrayPrimitive(fetch::vm::VM * /*vm*/, vm::Ptr< vm::Array< T > > const &g)
{
  std::cout << "[";
  for(std::size_t i=0; i < g->elements.size(); ++i )
  {
    if(i!=0)
    {
      std::cout << ", ";
    }      
    std::cout << g->elements[i];
  }
  std::cout << "]";
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
  module.CreateFreeFunction("Print", &PrintDouble);
  module.CreateFreeFunction("PrintLn", &PrintLnDouble);  
  module.CreateFreeFunction("Print", &PrintFloat);
  module.CreateFreeFunction("PrintLn", &PrintLnFloat);  

  module.CreateFreeFunction("Print", &PrintArrayPrimitive<int32_t>);  
  module.CreateFreeFunction("Print", &PrintArrayPrimitive<int64_t>);
  module.CreateFreeFunction("Print", &PrintArrayPrimitive<uint32_t>);  
  module.CreateFreeFunction("Print", &PrintArrayPrimitive<uint64_t>);
  module.CreateFreeFunction("Print", &PrintArrayPrimitive<float>);  
  module.CreateFreeFunction("Print", &PrintArrayPrimitive<double>);  
  }


}
}