//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "vm/module.hpp"
#include "vm_modules/ledger/random.hpp"

namespace fetch {
namespace vm_modules {
namespace ledger {

namespace {

uint32_t GetGeneratorResetValue(std::mt19937 & generator)
{
  std::uniform_int_distribution<uint32_t>  distr(50, 10000);
  return distr(generator);
}

} // namespace

using namespace fetch::vm;

RandomUniform::RandomUniform(VM *vm, vm::TypeId type_id)
  : Object{vm, type_id}
  , generator_{random_device_()}
{
  reset_ = GetGeneratorResetValue(generator_);
  counter_ = 0;
}

void RandomUniform::Bind(vm::Module &module)
{
  module.CreateClassType<RandomUniform>("RandomUniform").CreateMemberFunction("rand", &RandomUniform::rand);
}

int32_t RandomUniform::rand(int32_t low, int32_t high)
{
  if (counter_ >= reset_)
  {
    generator_.seed(random_device_());
    counter_ = 0;
    reset_ = GetGeneratorResetValue(generator_);
  }
  ++counter_;
  std::uniform_int_distribution<int32_t>  distr(low, high);
  return distr(generator_);
}

}  // namespace ledger
}  // namespace vm_modules
}  // namespace fetch
