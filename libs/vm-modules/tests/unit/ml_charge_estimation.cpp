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

#include "vm_modules/math/tensor.hpp"
#include "vm_modules/ml/model/model.hpp"
#include "vm_modules/ml/model/model_estimator.hpp"
#include "vm_modules/vm_factory.hpp"

#include "gmock/gmock.h"

#include <sstream>

using namespace fetch::vm;

namespace {

using VMModel        = fetch::vm_modules::ml::model::VMModel;
using VMTensor       = fetch::vm_modules::math::VMTensor;
using ModelEstimator = fetch::vm_modules::ml::model::ModelEstimator;
using VMFactory      = fetch::vm_modules::VMFactory;
using VMPtr          = std::unique_ptr<VM>;

class MLChargeEstimationTests : public ::testing::Test
{
public:
  VMPtr        vm;
  Ptr<VMModel> model;

  void SetUp() override
  {
    // setup the VM
    auto module = VMFactory::GetModule(VMFactory::USE_SMART_CONTRACTS);
    vm          = std::make_unique<VM>(module.get());

    // create a VMModel
    model = CreateSequentialModel();
  }

  Ptr<String> CreateString(std::string const &str)
  {
    return Ptr<String>{new String{vm.get(), str}};
  }

  Ptr<Array<uint64_t>> CreateArray(std::vector<uint64_t> const &values)
  {
    std::size_t          size = values.size();
    Ptr<Array<uint64_t>> array =
        vm->CreateNewObject<Array<uint64_t>>(vm->GetTypeId<uint64_t>(), static_cast<int32_t>(size));
    ;

    for (std::size_t i{0}; i < size; ++i)
    {
      array->elements[i] = values[i];
    }

    return array;
  }

  // Ptr<VMTensor> CreateTensor(Ptr<Array<uint64_t>> const& shape)
  Ptr<VMTensor> CreateTensor(std::vector<uint64_t> const &shape)
  {
    return vm->CreateNewObject<VMTensor>(shape);
  }

  Ptr<VMModel> CreateSequentialModel()
  {
    auto model_category = CreateString("sequential");
    return vm->CreateNewObject<VMModel>(model_category);
  }
};

TEST_F(MLChargeEstimationTests, test)
{
  using SizeRef      = fetch::math::SizeType const &;
  using StringPtrRef = fetch::vm::Ptr<fetch::vm::String> const &;

  // set up data and labels
  std::vector<uint64_t> data_shape{10, 1000};
  std::vector<uint64_t> label_shape{1, 1000};
  auto                  data  = CreateTensor(data_shape);
  auto                  label = CreateTensor(label_shape);

  // set up a model
  auto model = CreateSequentialModel();
  model->AddLayer<SizeRef, SizeRef, StringPtrRef>(
      CreateString("dense"), 10, 10,
      CreateString("relu"));  // input_size, hidden_1_size
  model->AddLayer<SizeRef, SizeRef, StringPtrRef>(
      CreateString("dense"), 10, 10,
      CreateString("relu"));                                        // hidden_1_size, hidden_2_size
  model->AddLayer<SizeRef, SizeRef>(CreateString("dense"), 10, 1);  // hidden_2_size, label_size
  model->CompileSequential(CreateString("mse"), CreateString("adam"));

  // train the model
  model->Fit(data, label, 32);  // batch_size

  // get loss value
  /*auto loss* = */ model->Evaluate();

  // make a prediction
  /*Ptr<VMTensor> prediction =*/model->Predict(data);

  ASSERT_TRUE(1 != 1);
}

}  // namespace
