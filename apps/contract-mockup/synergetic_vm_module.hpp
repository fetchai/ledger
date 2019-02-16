#pragma once

#include "crypto_rng.hpp"
#include "exp.hpp"
#include "byte_array_wrapper.hpp"
#include "print.hpp"
#include "item.hpp"
#include "dummy.hpp"
#include "dag_accessor.hpp"
#include "length.hpp"

namespace fetch
{
namespace consensus
{

void CreateConensusVMModule(fetch::vm::Module &module)
{
    fetch::modules::CryptoRNG::Bind(module);
    fetch::modules::ByteArrayWrapper::Bind(module);
    fetch::modules::ItemWrapper::Bind(module);
    fetch::modules::DAGWrapper::Bind(module);

    fetch::modules::BindExp(module);
    fetch::modules::BindPrint(module);
    fetch::modules::BindLen(module);
}

}
}