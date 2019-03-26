#pragma once
#include "vm_modules/core/byte_array_wrapper.hpp"
#include "core/byte_array/encoders.hpp"

namespace fetch
{
namespace vm_modules
{

inline int32_t LenByteArray(fetch::vm::VM * /*vm*/, fetch::vm::Ptr<fetch::vm_modules::ByteArrayWrapper> const &s)
{
  return int32_t(s->byte_array().size());
}


inline int32_t LenString(fetch::vm::VM * /*vm*/, fetch::vm::Ptr<fetch::vm::String> const &s)
{
  return int32_t(s->str.size());
}

template< typename T >
inline int32_t LenArray(fetch::vm::VM * /*vm*/, vm::Ptr< vm::Array< T > > const & arr)
{
  return int32_t(arr->elements.size());
}

template< typename T >
inline int32_t LenArrayObj(fetch::vm::VM * /*vm*/, vm::Ptr< vm::Array< vm::Ptr< T > > > const & arr)
{
  return int32_t(arr->elements.size());
}


inline void BindLen(vm::Module &module)
{

  module.CreateFreeFunction("lengthOf", &LenByteArray);
  module.CreateFreeFunction("lengthOf", &LenString);
  module.CreateFreeFunction("lengthOf", &LenArray<int32_t>);  
  module.CreateFreeFunction("lengthOf", &LenArray<int64_t>);    
  module.CreateFreeFunction("lengthOf", &LenArray<uint32_t>);  
  module.CreateFreeFunction("lengthOf", &LenArray<uint64_t>);
  module.CreateFreeFunction("lengthOf", &LenArray<float>);
  module.CreateFreeFunction("lengthOf", &LenArray<double>);  
  module.CreateFreeFunction("lengthOf", &LenArrayObj<DAGNodeWrapper>);      
}


}
}