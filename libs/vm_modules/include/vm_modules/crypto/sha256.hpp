#pragma once

#include "vm_modules/core/byte_array_wrapper.hpp"
#include "crypto/sha256.hpp"
#include "vm/analyser.hpp"
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

#include "vm/vm.hpp"


namespace fetch
{
namespace vm_modules
{


class SHA256Wrapper : public fetch::vm::Object
{
public:
  using ElementType = uint8_t;

  template< typename T >
  using Ptr = fetch::vm::Ptr<T>;
  using String = fetch::vm::String;

  SHA256Wrapper()          = delete;
  virtual ~SHA256Wrapper() = default;

  static void Bind(vm::Module &module)
  {
    module.CreateClassType<SHA256Wrapper>("SHA256")
      .CreateTypeConstuctor<>()
      .CreateInstanceFunction("update", &SHA256Wrapper::UpdateString)
      .CreateInstanceFunction("update", &SHA256Wrapper::UpdateBuffer)      
      .CreateInstanceFunction("final", &SHA256Wrapper::Final)
      .CreateInstanceFunction("reset", &SHA256Wrapper::Reset);
  }

  void UpdateString(vm::Ptr< vm::String > const&  str)
  {
    hasher_.Update(str->str);
  }

  void UpdateBuffer(Ptr< ByteArrayWrapper > const&  buffer)
  {
    hasher_.Update(buffer->byte_array());
  }  

  void Reset()
  {
    hasher_.Reset();
  }

  Ptr< ByteArrayWrapper > Final()
  {
    return vm_->CreateNewObject<ByteArrayWrapper>(hasher_.Final());
  }

  SHA256Wrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
    : fetch::vm::Object(vm, type_id)
  {}

  static fetch::vm::Ptr<SHA256Wrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id)
  {
    return new SHA256Wrapper(vm, type_id );
  }

private:
  crypto::SHA256 hasher_;
};

}
}