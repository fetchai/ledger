#pragma once

#include "vm_modules/core/crypto_rng.hpp"
#include "vm_modules/math/exp.hpp"
#include "vm_modules/math/sqrt.hpp"
#include "vm_modules/polyfill/bitshifting.hpp"
#include "vm_modules/polyfill/bitwise_ops.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"
#include "vm_modules/core/print.hpp"
#include "vm_modules/ledger/dag_node_wrapper.hpp"
#include "vm_modules/ledger/dag_accessor.hpp"
#include "vm_modules/polyfill/length.hpp"

namespace fetch
{
namespace consensus
{

void CreateConensusVMModule(fetch::vm::Module &module)
{
  module.CreateTemplateInstantiationType<fetch::vm::Array, int32_t >(fetch::vm::TypeIds::IArray);
  module.CreateTemplateInstantiationType<fetch::vm::Array, int64_t >(fetch::vm::TypeIds::IArray);
  module.CreateTemplateInstantiationType<fetch::vm::Array, uint32_t >(fetch::vm::TypeIds::IArray);
  module.CreateTemplateInstantiationType<fetch::vm::Array, uint64_t >(fetch::vm::TypeIds::IArray);
  module.CreateTemplateInstantiationType<fetch::vm::Array, double >(fetch::vm::TypeIds::IArray);
  module.CreateTemplateInstantiationType<fetch::vm::Array, float >(fetch::vm::TypeIds::IArray);

  fetch::modules::CryptoRNG::Bind(module);
  fetch::modules::ByteArrayWrapper::Bind(module);
  fetch::modules::DAGNodeWrapper::Bind(module);

	module.CreateTemplateInstantiationType<fetch::vm::Array, fetch::vm::Ptr<modules::DAGNodeWrapper>>(fetch::vm::TypeIds::IArray);    

  fetch::modules::DAGWrapper::Bind(module);

  fetch::modules::BindExp(module);
  fetch::modules::BindSqrt(module);  
  fetch::vm_modules::CreatePrint(module);
  fetch::modules::BindLen(module);
  fetch::modules::BindBitShift(module);
  fetch::modules::BindBitwiseOps(module);
}

}
}