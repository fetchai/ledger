#pragma once
#include "core/script/variant.hpp"
#include "ledger/chaincode/contract.hpp"
#include "vm/module.hpp"

namespace fetch {
namespace ledger {

template <typename S>
std::unique_ptr<vm::Module> CreateVMDefinition(S *sc)
{
  using SmartContract = S;

  std::unique_ptr<vm::Module> module;
  module.reset(new vm::Module());

  module->ExportClass< byte_array::ByteArray >("ByteArray")
    .Constructor<std::string>();

  module->ExportClass< script::Variant >("Variant")
    .Constructor();
/*    
    .Export("Get", [](int32_t const &index)) {
      script::Variant ret;
      if()
      return ret;
    })
    .Export("Set", [](int32_t const &index, Variant const &val)) -> bool {
      script::Variant ret;
      if()
      return ret;
    });

*/
  module->ExportFunction("toString",[](script::Variant const &var) -> std::string {
    if(var.is_string())
    {
        return var.As<std::string>();
    }
    return std::string("");
  });

  module->ExportFunction("toInt32",[](script::Variant const &var) -> int32_t {
    if(var.is_int())
    {
        return int32_t(var.As<int64_t>());
    }
    return int32_t(0);
  });  


  module->ExportFunction("toInt64",[](script::Variant const &var) -> int64_t {
    if(var.is_int())
    {
        return var.As<int64_t>();
    }
    return int64_t(0);
  });  

  module->ExportFunction("toBool",[](script::Variant const &var) -> bool {
    if(var.is_int())
    {
        return int32_t(var.As<bool>());
    }
    return bool(false);
  });  

  module->ExportFunction("toVariant",[](std::string const &val) -> script::Variant {
    script::Variant ret;
    ret = byte_array::ByteArray(val);
    return ret;
  });  

  module->ExportFunction("toVariant",[](int64_t const &val) -> script::Variant {
    script::Variant ret = val;
    return ret;
  });  

  module->ExportFunction("toVariant",[](int32_t const &val) -> script::Variant {
    script::Variant ret = val;
    return ret;
  });  

  module->ExportFunction("toVariant",[](bool const &val) -> script::Variant {
    script::Variant ret = val;
    return ret;
  });  

  module->ExportClass<SmartContract>("Ledger")
    .ExportStaticFunction("Print", [](std::string const &s) { std::cout << "LEDGER: " << s << std::endl; })  
    .ExportStaticFunction("GetOrCreateRecord", [](std::string const &address) -> script::Variant { 
      script::Variant ret;
//        sc->GetOrCreateStateRecord(ret, address);
      return ret;
    })
    .ExportStaticFunction("GetRecord", []() -> int { return 0; })
    .ExportStaticFunction("SetRecord", []() -> int { return 0; });

  return module;
}

}  // namespace ledger
}  // namespace fetch