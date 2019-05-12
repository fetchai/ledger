#pragma once

#include "vm_modules/core/crypto_rng.hpp"
#include "vm_modules/math/exp.hpp"
#include "vm_modules/math/sqrt.hpp"
#include "vm_modules/polyfill/bitshifting.hpp"
#include "vm_modules/polyfill/bitwise_ops.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"
#include "vm_modules/core/print.hpp"
#include "vm_modules/core/type_convert.hpp"
#include "vm_modules/ledger/dag_node_wrapper.hpp"
#include "vm_modules/ledger/dag_accessor.hpp"
#include "vm_modules/ledger/chain_state.hpp"
#include "vm_modules/polyfill/length.hpp"
#include "vm_modules/math/bignumber.hpp"
#include "vm_modules/crypto/sha256.hpp"

namespace fetch
{
namespace consensus
{

inline void CreateConensusVMModule(fetch::vm::Module &module)
{
  module.CreateTemplateInstantiationType<fetch::vm::Array, int32_t >(fetch::vm::TypeIds::IArray);
  module.CreateTemplateInstantiationType<fetch::vm::Array, int64_t >(fetch::vm::TypeIds::IArray);
  module.CreateTemplateInstantiationType<fetch::vm::Array, uint32_t >(fetch::vm::TypeIds::IArray);
  module.CreateTemplateInstantiationType<fetch::vm::Array, uint64_t >(fetch::vm::TypeIds::IArray);
  module.CreateTemplateInstantiationType<fetch::vm::Array, double >(fetch::vm::TypeIds::IArray);
  module.CreateTemplateInstantiationType<fetch::vm::Array, float >(fetch::vm::TypeIds::IArray);
  module.CreateTemplateInstantiationType<fetch::vm::Array, fetch::vm::Ptr< fetch::vm::String > >(fetch::vm::TypeIds::IArray);  

  fetch::vm_modules::ByteArrayWrapper::Bind(module);
  fetch::vm_modules::CryptoRNG::Bind(module);
  fetch::vm_modules::BigNumberWrapper::Bind(module);
  fetch::vm_modules::SHA256Wrapper::Bind(module);
  fetch::vm_modules::DAGNodeWrapper::Bind(module);

	module.CreateTemplateInstantiationType<fetch::vm::Array, fetch::vm::Ptr<vm_modules::DAGNodeWrapper>>(fetch::vm::TypeIds::IArray);    
  module.CreateTemplateInstantiationType<fetch::vm::Array, fetch::vm::Ptr<vm_modules::ByteArrayWrapper>>(fetch::vm::TypeIds::IArray);      

  fetch::vm_modules::DAGWrapper::Bind(module);

  fetch::vm_modules::BindExp(module);
  fetch::vm_modules::BindSqrt(module);  
  fetch::vm_modules::CreatePrint(module);
  fetch::vm_modules::BindBitShift(module);
  fetch::vm_modules::BindBitwiseOps(module);
  fetch::vm_modules::CreateToString(module);
  fetch::vm_modules::CreateToBool(module);  

  fetch::vm_modules::BindLen(module);  
  fetch::vm_modules::CreateChainFunctions(module);
}

}
}