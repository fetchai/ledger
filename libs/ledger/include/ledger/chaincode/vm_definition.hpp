#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "ledger/chaincode/contract.hpp"
#include "variant/variant.hpp"
#include "vm/module.hpp"

namespace fetch {
namespace ledger {
/*
template <typename S>
std::unique_ptr<vm::Module> CreateVMDefinition(S * smart_contract_instance =
nullptr)
{
  using SmartContract = S;

  std::unique_ptr<vm::Module> module;
  module = std::make_unique<vm::Module>();

  module->ExportClass<byte_array::ByteArray>("ByteArray").Constructor<std::string>();

  module->ExportClass<variant::Variant>("Variant").Constructor();

  module->ExportFunction("toString", [](variant::Variant const &var) ->
std::string {
    if (var.Is<std::string>())
    {
      return var.As<std::string>();
    }
    return std::string("");
  });

  module->ExportFunction("toInt32", [](variant::Variant const &var) -> int32_t {
    if (var.Is<int32_t>())
    {
      return var.As<int32_t>();
    }
    return int32_t(0);
  });

  module->ExportFunction("toInt64", [](variant::Variant const &var) -> int64_t {
    if (var.Is<int64_t>())
    {
      return var.As<int64_t>();
    }
    return int64_t(0);
  });

  module->ExportFunction("toBool", [](variant::Variant const &var) -> bool {
    if (var.Is<bool>())
    {
      return var.As<bool>();
    }
    return bool(false);
  });

  module->ExportFunction("toVariant", [](std::string const &val) ->
variant::Variant {
    return variant::Variant{val.c_str()};
  });

  module->ExportFunction(
      "toVariant", [](int64_t const &val) -> variant::Variant { return
variant::Variant{val}; });

  module->ExportFunction(
      "toVariant", [](int32_t const &val) -> variant::Variant { return
variant::Variant{val}; });

  module->ExportFunction("toVariant",
                         [](bool const &val) -> variant::Variant { return
variant::Variant{val}; });

  module->ExportClass<SmartContract>("Ledger")
      .ExportStaticFunction("GetOrCreateRecord",
                            [](std::string const &address) -> variant::Variant {
                              variant::Variant ret;

                              return ret;
                            })
      .ExportStaticFunction("GetRecord", []() -> int { return 0; })
      .ExportStaticFunction("SetRecord", []() -> int { return 0; });

  return module;
}
*/

}  // namespace ledger
}  // namespace fetch
