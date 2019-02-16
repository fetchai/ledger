#pragma once
#include "byte_array_wrapper.hpp"
#include "core/byte_array/encoders.hpp"
#include "item.hpp"

namespace fetch
{
namespace modules
{

int32_t LenByteArray(fetch::vm::VM * /*vm*/, fetch::vm::Ptr<fetch::modules::ByteArrayWrapper> const &s)
{
  return int32_t(s->byte_array().size());
}


int32_t LenString(fetch::vm::VM * /*vm*/, fetch::vm::Ptr<fetch::vm::String> const &s)
{
  return int32_t(s->str.size());
}

template< typename T >
int32_t LenArray(fetch::vm::VM * /*vm*/, vm::Ptr< vm::Array< T > > const & arr)
{
  return int32_t(arr->elements.size());
}

template< typename T >
int32_t LenArrayObj(fetch::vm::VM * /*vm*/, vm::Ptr< vm::Array< vm::Ptr< T > > > const & arr)
{
  return int32_t(arr->elements.size());
}


void BindLen(vm::Module &module)
{
  module.CreateTemplateInstantiationType<fetch::vm::Array, int32_t >(fetch::vm::TypeIds::IArray);
  module.CreateTemplateInstantiationType<fetch::vm::Array, int64_t >(fetch::vm::TypeIds::IArray);
  module.CreateTemplateInstantiationType<fetch::vm::Array, uint32_t >(fetch::vm::TypeIds::IArray);
  module.CreateTemplateInstantiationType<fetch::vm::Array, uint64_t >(fetch::vm::TypeIds::IArray);
  
  module.CreateFreeFunction("lengthOf", &LenByteArray);
  module.CreateFreeFunction("lengthOf", &LenString);
  module.CreateFreeFunction("lengthOf", &LenArray<int64_t>);  
  module.CreateFreeFunction("lengthOf", &LenArray<int64_t>);    
  module.CreateFreeFunction("lengthOf", &LenArray<uint32_t>);  
  module.CreateFreeFunction("lengthOf", &LenArray<uint64_t>);    
  module.CreateFreeFunction("lengthOf", &LenArrayObj<ItemWrapper>);      
}


}
}