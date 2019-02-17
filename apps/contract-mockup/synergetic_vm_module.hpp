#pragma once

#include "crypto_rng.hpp"
#include "exp.hpp"
#include "sqrt.hpp"
#include "bitshifting.hpp"
#include "bitwise_ops.hpp"
#include "byte_array_wrapper.hpp"
#include "print.hpp"
#include "dag_node_wrapper.hpp"
#include "dag_accessor.hpp"
#include "length.hpp"

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
  fetch::modules::BindPrint(module);
  fetch::modules::BindLen(module);
  fetch::modules::BindBitShift(module);
  fetch::modules::BindBitwiseOps(module);
}

}
}