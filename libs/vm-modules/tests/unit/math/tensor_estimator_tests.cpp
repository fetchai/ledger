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

namespace {

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

  SizeType MinDimSize()
  {
    return SizeType(1);
  }

  SizeType MaxDimSize()
  {
    return SizeType(20);
  }

  SizeType DimStep()
  {
    return SizeType(7);
  }

  SizeType MinDims()
  {
    return SizeType(1);
  }

  SizeType MaxDims()
  {
    return SizeType(8);
  }

  SizeType DimsStep()
  {
    return SizeType(1);
  }
};

// sanity check that estimator behaves as intended
TEST_F(MathTensorEstimatorTests, tensor_estimator_min_test)
{
  fetch::vm::TypeId type_id = 0;

  SizeType min_dim_size = MinDimSize();
  SizeType max_dim_size = MaxDimSize();
  SizeType dim_step     = DimStep();

  SizeType min_dims  = MinDims();
  SizeType max_dims  = MaxDims();
  SizeType dims_step = DimsStep();

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

      SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_shape);
      SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_shape);

      ChargeAmount val =
          static_cast<ChargeAmount>(VmTensorEstimator::MIN_PADDED_SIZE_COEF() * padded_size +
                                    VmTensorEstimator::MIN_SIZE_COEF() * size +
                                    VmTensorEstimator::MIN_CONST_COEF()) *
          fetch::vm::COMPUTE_CHARGE_COST;

      EXPECT_TRUE(tensor_estimator.Min() == val);
    }
  }
}

// sanity check that estimator behaves as intended
TEST_F(MathTensorEstimatorTests, tensor_estimator_max_test)
{
  fetch::vm::TypeId type_id = 0;

  SizeType min_dim_size = MinDimSize();
  SizeType max_dim_size = MaxDimSize();
  SizeType dim_step     = DimStep();

  SizeType min_dims  = MinDims();
  SizeType max_dims  = MaxDims();
  SizeType dims_step = DimsStep();

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

      SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_shape);
      SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_shape);

      ChargeAmount val =
          static_cast<ChargeAmount>(VmTensorEstimator::MAX_PADDED_SIZE_COEF() * padded_size +
                                    VmTensorEstimator::MAX_SIZE_COEF() * size +
                                    VmTensorEstimator::MAX_CONST_COEF()) *
          fetch::vm::COMPUTE_CHARGE_COST;

      EXPECT_TRUE(tensor_estimator.Max() == val);
    }
  }
}

// sanity check that estimator behaves as intended
TEST_F(MathTensorEstimatorTests, tensor_estimator_sum_test)
{
  fetch::vm::TypeId type_id = 0;

  SizeType min_dim_size = MinDimSize();
  SizeType max_dim_size = MaxDimSize();
  SizeType dim_step     = DimStep();

  SizeType min_dims  = MinDims();
  SizeType max_dims  = MaxDims();
  SizeType dims_step = DimsStep();

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

      SizeType padded_size = fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_shape);
      SizeType size        = fetch::math::Tensor<DataType>::SizeFromShape(tensor_shape);

      ChargeAmount val =
          static_cast<ChargeAmount>(VmTensorEstimator::SUM_PADDED_SIZE_COEF() * padded_size +
                                    VmTensorEstimator::SUM_SIZE_COEF() * size +
                                    VmTensorEstimator::SUM_CONST_COEF()) *
          fetch::vm::COMPUTE_CHARGE_COST;

      EXPECT_TRUE(tensor_estimator.Sum() == val);
    }
  }
}

TEST_F(MathTensorEstimatorTests, tensor_estimator_transpose_test)
{
  for (SizeType n_dims = MinDims(); n_dims < MaxDims(); n_dims += DimsStep())
  {
    for (SizeType cur_dim_size = MinDimSize(); cur_dim_size < MaxDimSize();
         cur_dim_size += DimStep())
    {
      std::vector<SizeType> const tensor_shape(n_dims, cur_dim_size);

      MathTensor        tensor{tensor_shape};
      VmTensor          vm_tensor(&toolkit.vm(), fetch::vm::TypeIds::Unknown, tensor);
      VmTensorEstimator tensor_estimator(vm_tensor);

      ChargeAmount const expected_charge_amount = tensor.size();
      EXPECT_EQ(tensor_estimator.Transpose(), expected_charge_amount);
    }
  }
}

}  // namespace
