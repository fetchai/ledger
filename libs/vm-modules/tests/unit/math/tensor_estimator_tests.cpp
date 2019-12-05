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

#include "gmock/gmock.h"
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/math/tensor/tensor_estimator.hpp"
#include "vm_test_toolkit.hpp"

namespace math_tensor_estimator_tests {

using SizeType          = fetch::math::SizeType;
using DataType          = fetch::vm_modules::math::DataType;
using VmPtr             = fetch::vm::Ptr<fetch::vm::String>;
using MathTensor        = fetch::math::Tensor<DataType>;
using VmTensor          = fetch::vm_modules::math::VMTensor;
using VmTensorEstimator = fetch::vm_modules::math::TensorEstimator;

class MathTensorEstimatorTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

// sanity check that estimator behaves as intended
TEST_F(MathTensorEstimatorTests, tensor_min_test)
{
  fetch::vm::TypeId type_id = 0;

  SizeType min_dim_size = 1;
  SizeType max_dim_size = 100;
  SizeType dim_step     = 17;

  SizeType min_dims  = 1;
  SizeType max_dims  = 8;
  SizeType dims_step = 1;

  for (SizeType n_dims = min_dims; n_dims < max_dims; n_dims += dims_step)
  {
    for (SizeType cur_dim_size = min_dim_size; cur_dim_size < max_dim_size;
         cur_dim_size += dim_step)
    {
      std::vector<SizeType> tensor_shape{};
      for (SizeType cur_dim = 0; cur_dim < n_dims; ++cur_dim)
      {
        tensor_shape.emplace_back(cur_dim_size);
      }

      MathTensor        tensor{tensor_shape};
      VmTensor          vm_tensor(&toolkit.vm(), type_id, tensor);
      VmTensorEstimator tensor_estimator(vm_tensor);

      ChargeAmount val = tensor.size();
      EXPECT_TRUE(tensor_estimator.Min() == val);
    }
  }
}

}  // namespace math_tensor_estimator_tests
