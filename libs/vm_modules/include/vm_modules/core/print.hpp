#pragma once
#include "byte_array_wrapper.hpp"
#include "core/byte_array/encoders.hpp"

namespace fetch
{
namespace vm_modules
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



void CreatePrint(vm::Module& module)
{
  module.CreateFreeFunction("print", &PrintByteArray);
  module.CreateFreeFunction("printLn", &PrintLnByteArray);
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
}

void CreatePrint(std::shared_ptr<vm::Module> module)
{
  CreatePrint(*module.get());
}



}
}