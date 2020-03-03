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

#include "vm_modules/ml/dataloaders/dataloader.hpp"
#include "vm_modules/ml/utilities/conversion_utilities.hpp"
#include "vm_modules/vm_factory.hpp"

#include "gmock/gmock.h"

namespace {

using SizeType        = fetch::math::SizeType;
using DataType        = fetch::vm_modules::math::DataType;
using VmStringPtr     = fetch::vm::Ptr<fetch::vm::String>;
using ChargeAmount    = fetch::vm::ChargeAmount;
using VMTensor        = fetch::vm_modules::math::VMTensor;
using VMTensorPtr     = fetch::vm::Ptr<fetch::vm_modules::math::VMTensor>;
using VMPtr           = std::shared_ptr<fetch::vm::VM>;
using VMTensorArray   = fetch::vm::Array<VMTensorPtr>;
using VMDataLoaderPtr = fetch::vm::Ptr<fetch::vm_modules::ml::VMDataLoader>;

class VMDataloaderTests : public ::testing::Test
{
public:
  VMPtr           vm;
  VMDataLoaderPtr dl;

  void SetUp() override
  {
    // setup the VM
    using VMFactory = fetch::vm_modules::VMFactory;
    auto module     = VMFactory::GetModule(fetch::vm_modules::VMFactory::USE_ALL);
    vm              = std::make_shared<fetch::vm::VM>(module.get());
    dl              = vm->CreateNewObject<fetch::vm_modules::ml::VMDataLoader>(
        fetch::vm_modules::ml::utilities::VMStringConverter(vm.get(), "tensor"));
  }
};

TEST_F(VMDataloaderTests, vmdataloader_addtensordata)
{
  auto data = std::vector<fetch::math::Tensor<DataType>>();
  data.emplace_back(fetch::math::Tensor<DataType>({7, 3}));
  data.emplace_back(fetch::math::Tensor<DataType>({5, 3}));

  auto data_vmarray   = fetch::vm_modules::ml::utilities::VMArrayConverter(vm.get(), data);
  auto label_vmtensor = fetch::vm_modules::ml::utilities::VMTensorConverter(vm.get(), {2, 3});

  EXPECT_NO_THROW(dl->AddDataByData(data_vmarray, label_vmtensor));

  ChargeAmount charge = dl->EstimateAddDataByData(data_vmarray, label_vmtensor);
  EXPECT_EQ(charge, 3008);

  EXPECT_NO_THROW(dl->GetNext());
  charge = dl->EstimateGetNext();
  EXPECT_EQ(charge, 19);

  EXPECT_NO_THROW(dl->IsDone());
  charge = dl->EstimateIsDone();
  EXPECT_EQ(charge, 3);
}
}  // namespace
