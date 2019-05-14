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

#include "core/random/lfg.hpp"
#include "vm/analyser.hpp"
#include "vm/common.hpp"
#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm_modules {

class CryptoRNG : public fetch::vm::Object
{
public:
  CryptoRNG()          = delete;
  virtual ~CryptoRNG() = default;

  static void Bind(vm::Module & /*module*/)
  {
    /*
    module.CreateClassType<CryptoRNG>("CryptoRNG")
        .CreateTypeConstuctor<uint64_t>()
        .CreateInstanceFunction("next", &CryptoRNG::Next)
        .CreateInstanceFunction("nextAsFloat", &CryptoRNG::NextAsFloat);
    */
  }

  CryptoRNG(fetch::vm::VM *vm, fetch::vm::TypeId type_id, uint64_t seed)
    : fetch::vm::Object(vm, type_id)
    , rng_(seed)
  {}

  static fetch::vm::Ptr<CryptoRNG> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                               uint64_t seed)
  {
    return new CryptoRNG(vm, type_id, seed);
  }

  uint64_t Next()
  {
    return rng_();
  }

  double NextAsFloat()  // TODO: Replace with fixed point
  {
    return rng_.AsDouble();
  }

private:
  // TODO(tfr): Replace with "real" ledger based crypto rng
  random::LaggedFibonacciGenerator<> rng_;
};

}  // namespace vm_modules
}  // namespace fetch
