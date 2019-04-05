#pragma once

#include "vm_modules/core/byte_array_wrapper.hpp"
#include "math/bignumber.hpp"
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

// TODO: Make templated
class BigNumberWrapper : public fetch::vm::Object
{
public:
  using SizeType = uint64_t;

  template< typename T >
  using Ptr = fetch::vm::Ptr<T>;
  using String = fetch::vm::String;

  BigNumberWrapper()          = delete;
  virtual ~BigNumberWrapper() = default;

  static void Bind(vm::Module &module)
  {
    module.CreateClassType<BigNumberWrapper>("BigUInt")
      .CreateTypeConstuctor< Ptr< ByteArrayWrapper > >()
      .CreateInstanceFunction("toBuffer", &BigNumberWrapper::ToBuffer)
      .CreateInstanceFunction("increase", &BigNumberWrapper::Increase)
      .CreateInstanceFunction("lessThan", &BigNumberWrapper::LessThan)
      .CreateInstanceFunction("logValue",  &BigNumberWrapper::LogValue)
      .CreateInstanceFunction("toFloat64",  &BigNumberWrapper::ToFloat64)
      .CreateInstanceFunction("toInt32",  &BigNumberWrapper::ToInt32)      
      .CreateInstanceFunction("size", &BigNumberWrapper::size);
  }

  BigNumberWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, byte_array::ByteArray data)
    : fetch::vm::Object(vm, type_id)
    , number_(data)
  {}

  static fetch::vm::Ptr<BigNumberWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id, fetch::vm::Ptr<ByteArrayWrapper> const &ba)
  {
    return new BigNumberWrapper(vm, type_id, ba->byte_array() );
  }

  double ToFloat64()
  {
    return math::ToDouble(number_);
  }

  int32_t ToInt32()
  {
    union {
      uint8_t bytes[4];
      int32_t value;
    } x;
    x.bytes[0] = number_[0];
    x.bytes[1] = number_[1];
    x.bytes[2] = number_[2];
    x.bytes[3] = number_[3];
    return x.value;
  }  

  double LogValue()
  {
    return math::Log(number_);
  }  


  fetch::vm::Ptr<ByteArrayWrapper> ToBuffer()
  {
    return vm_->CreateNewObject< ByteArrayWrapper >(number_.Copy());
  }

  bool LessThan(Ptr< BigNumberWrapper > const & other)
  {
    // TODO'
    return true;
  }

  void Increase()
  {
    ++number_;
  }

  SizeType size() const 
  {
    return number_.size();
  }
private:
  math::BigUnsigned number_;
};

}
}