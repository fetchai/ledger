#pragma once
#include "vm/module.hpp"

namespace fetch {
namespace ledger {

template <typename S>
std::unique_ptr<vm::Module> CreateVMDefinition(S *sc = nullptr)
{
  using SmartContract = S;

  std::unique_ptr<vm::Module> module;
  module.reset(new vm::Module());
  module->ExportClass<SmartContract>("Ledger")
      .ExportStaticFunction("GetOrCreateRecord", []() -> int { return 0; })
      .ExportStaticFunction("GetRecord", []() -> int { return 0; })
      .ExportStaticFunction("SetRecord", []() -> int { return 0; });

  return module;
}

}  // namespace ledger
}  // namespace fetch