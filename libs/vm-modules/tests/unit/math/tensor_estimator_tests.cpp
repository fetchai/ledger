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

#include "gmock/gmock.h"
#include "vm/array.hpp"
#include "vm_modules/math/tensor/tensor.hpp"
#include "vm_modules/math/tensor/tensor_estimator.hpp"
#include "vm_test_toolkit.hpp"

namespace {

using SizeType          = fetch::math::SizeType;
using DataType          = fetch::vm_modules::math::DataType;
using MathTensor        = fetch::math::Tensor<DataType>;
using VmTensor          = fetch::vm_modules::math::VMTensor;
using VmTensorEstimator = fetch::vm_modules::math::TensorEstimator;

using ShapeFrom = std::vector<SizeType>;
using ShapeTo   = std::vector<SizeType>;
using ShapePair = std::pair<ShapeFrom, ShapeTo>;
std::vector<ShapePair> const VALID_TRANSFORMATIONS{
    {{1, 1, 1, 1}, {1}},                                 //
    {{2, 2, 2, 2}, {4, 4}},                              //
    {{4, 4, 4, 4}, {16, 8, 2}},                          //
    {{4, 4, 4, 4}, {16, 4, 4, 1}},                       //
    {{8, 8, 8, 8}, {64, 8, 4, 2}},                       //
    {{16, 16, 16, 16}, {2, 2, 2, 2, 4, 4, 8, 2, 4, 4}},  //
};
std::vector<ShapePair> const EQUAL_TRANSFORMATIONS{
    {{1, 1, 1, 1}, {1, 1, 1, 1}},                                      //
    {{2, 2, 2, 2}, {2, 2, 2, 2}},                                      //
    {{2, 2, 2, 2, 4, 4, 8, 2, 4, 4}, {2, 2, 2, 2, 4, 4, 8, 2, 4, 4}},  //
    {{64, 8, 4, 2}, {64, 8, 4, 2}},                                    //
    {{1, 2, 3, 4, 5, 6}, {1, 2, 3, 4, 5, 6}},                          //
};
std::vector<ShapePair> const INVALID_TRANSFORMATIONS{
    {{1, 1, 1, 1}, {0}},                                 //
    {{2, 2, 2, 2}, {3, 4}},                              //
    {{4, 4, 4, 4}, {19, 8, 2}},                          //
    {{4, 4, 4, 4}, {0, 4, 4, 1}},                        //
    {{8, 8, 8, 8}, {64, 8, 4, 1, 1}},                    //
    {{16, 16, 16, 16}, {2, 2, 2, 3, 4, 4, 8, 2, 4, 4}},  //
};

class MathTensorEstimatorTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};

  fetch::vm::Array<SizeType> GetShapeArray(std::vector<SizeType> const &shape)
  {
    using namespace fetch::vm;
    Array<SizeType> a(&toolkit.vm(), TypeIds::Unknown, TypeIds::Int32, int32_t(0));
    for (auto const &el : shape)
    {
      a.Append(TemplateParameter1(el, TypeIds::Int32));
    }
    return a;
  }

  SizeType MinDimSize()
  {
    return SizeType(1);
  }

  SizeType MaxDimSize()
  {
    return SizeType(26);
  }

  SizeType DimSizeStep()
  {
    return SizeType(5);
  }

  SizeType MinDims()
  {
    return SizeType(1);
  }

  SizeType MaxDims()
  {
    return SizeType(6);
  }

  SizeType DimsStep()
  {
    return SizeType(1);
  }

  ChargeAmount GetReferenceReshapeEstimation(ShapeTo const &from_shape, ShapeTo const &new_shape)
  {
    DataType from_size =
        static_cast<DataType>(fetch::math::Tensor<DataType>::PaddedSizeFromShape(from_shape));
    DataType padded_size =
        static_cast<DataType>(fetch::math::Tensor<DataType>::PaddedSizeFromShape(new_shape));

    return static_cast<ChargeAmount>(VmTensorEstimator::RESHAPE_PADDED_SIZE_FROM_COEF * from_size +
                                     VmTensorEstimator::RESHAPE_PADDED_SIZE_TO_COEF * padded_size +
                                     VmTensorEstimator::RESHAPE_CONST_COEF) *
           fetch::vm::COMPUTE_CHARGE_COST;
  }
};

// sanity check that estimator behaves as intended
TEST_F(MathTensorEstimatorTests, tensor_estimator_min_test)
{
  fetch::vm::TypeId type_id = 0;

  SizeType min_dim_size = MinDimSize();
  SizeType max_dim_size = MaxDimSize();
  SizeType dim_step     = DimSizeStep();

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

      DataType padded_size =
          static_cast<DataType>(fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_shape));
      DataType size =
          static_cast<DataType>(fetch::math::Tensor<DataType>::SizeFromShape(tensor_shape));

      ChargeAmount const expected_charge =
          1 + static_cast<ChargeAmount>(VmTensorEstimator::MIN_PADDED_SIZE_COEF * padded_size +
                                        VmTensorEstimator::MIN_SIZE_COEF * size +
                                        VmTensorEstimator::MIN_CONST_COEF) *
                  fetch::vm::COMPUTE_CHARGE_COST;

      EXPECT_EQ(tensor_estimator.Min(), expected_charge);
    }
  }
}

// sanity check that estimator behaves as intended
TEST_F(MathTensorEstimatorTests, tensor_estimator_max_test)
{
  fetch::vm::TypeId type_id = 0;

  SizeType min_dim_size = MinDimSize();
  SizeType max_dim_size = MaxDimSize();
  SizeType dim_step     = DimSizeStep();

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

      DataType padded_size =
          static_cast<DataType>(fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_shape));
      DataType size =
          static_cast<DataType>(fetch::math::Tensor<DataType>::SizeFromShape(tensor_shape));

      ChargeAmount const expected_charge =
          1 + static_cast<ChargeAmount>(VmTensorEstimator::MAX_PADDED_SIZE_COEF * padded_size +
                                        VmTensorEstimator::MAX_SIZE_COEF * size +
                                        VmTensorEstimator::MAX_CONST_COEF) *
                  fetch::vm::COMPUTE_CHARGE_COST;

      EXPECT_EQ(tensor_estimator.Max(), expected_charge);
    }
  }
}

// sanity check that estimator behaves as intended
TEST_F(MathTensorEstimatorTests, tensor_estimator_sum_test)
{
  fetch::vm::TypeId type_id = 0;

  SizeType min_dim_size = MinDimSize();
  SizeType max_dim_size = MaxDimSize();
  SizeType dim_step     = DimSizeStep();

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

      DataType padded_size =
          static_cast<DataType>(fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_shape));
      DataType size =
          static_cast<DataType>(fetch::math::Tensor<DataType>::SizeFromShape(tensor_shape));

      ChargeAmount const expected_charge =
          1 + static_cast<ChargeAmount>(VmTensorEstimator::SUM_PADDED_SIZE_COEF * padded_size +
                                        VmTensorEstimator::SUM_SIZE_COEF * size +
                                        VmTensorEstimator::SUM_CONST_COEF) *
                  fetch::vm::COMPUTE_CHARGE_COST;

      EXPECT_EQ(tensor_estimator.Sum(), expected_charge);
    }
  }
}

TEST_F(MathTensorEstimatorTests, tensor_estimator_transpose_test)
{
  SizeType const n_dims = 2;
  for (SizeType cur_dim_size = MinDimSize(); cur_dim_size < MaxDimSize();
       cur_dim_size += DimSizeStep())
  {
    std::vector<SizeType> const tensor_shape(n_dims, cur_dim_size);
    std::vector<SizeType> const new_shape(n_dims, cur_dim_size);

    MathTensor        tensor{tensor_shape};
    VmTensor          vm_tensor(&toolkit.vm(), fetch::vm::TypeIds::Unknown, tensor);
    VmTensorEstimator tensor_estimator(vm_tensor);

    DataType padded_from_size =
        static_cast<DataType>(fetch::math::Tensor<DataType>::PaddedSizeFromShape(tensor_shape));
    DataType padded_to_size =
        static_cast<DataType>(fetch::math::Tensor<DataType>::PaddedSizeFromShape(new_shape));

    ChargeAmount expected_charge{0};
    if (tensor_shape == new_shape)
    {
      expected_charge = VmTensorEstimator::LOW_CHARGE_CONST_COEF * fetch::vm::COMPUTE_CHARGE_COST;
    }
    else
    {
      expected_charge = static_cast<ChargeAmount>(
                            VmTensorEstimator::RESHAPE_PADDED_SIZE_FROM_COEF * padded_from_size +
                            VmTensorEstimator::RESHAPE_PADDED_SIZE_TO_COEF * padded_to_size +
                            VmTensorEstimator::RESHAPE_CONST_COEF) *
                        fetch::vm::COMPUTE_CHARGE_COST;
    }

    ChargeAmount const estimated_charge = tensor_estimator.Transpose();
    EXPECT_EQ(estimated_charge, expected_charge);
  }
}

TEST_F(MathTensorEstimatorTests, tensor_estimator_valid_reshape_test)
{
  using namespace fetch::vm;
  for (ShapePair const &shapes : VALID_TRANSFORMATIONS)
  {
    ShapeFrom const   initial_shape = shapes.first;
    MathTensor        initial_tensor{initial_shape};
    VmTensor          vm_tensor(&toolkit.vm(), TypeIds::Unknown, initial_tensor);
    VmTensorEstimator tensor_estimator(vm_tensor);

    ShapeTo const      new_shape_raw = shapes.second;
    ChargeAmount const expected_charge =
        1 + GetReferenceReshapeEstimation(initial_shape, new_shape_raw);

    Array<SizeType>    shape_array      = GetShapeArray(new_shape_raw);
    Ptr<IArray>        new_shape_ptr    = Ptr<IArray>::PtrFromThis(&shape_array);
    ChargeAmount const estimated_charge = tensor_estimator.Reshape(new_shape_ptr);

    EXPECT_EQ(expected_charge, estimated_charge);
  }
}

TEST_F(MathTensorEstimatorTests, tensor_estimator_equal_reshape_test)
{
  using namespace fetch::vm;
  for (ShapePair const &shapes : EQUAL_TRANSFORMATIONS)
  {
    ShapeFrom const   initial_shape = shapes.first;
    MathTensor        initial_tensor{initial_shape};
    VmTensor          vm_tensor(&toolkit.vm(), TypeIds::Unknown, initial_tensor);
    VmTensorEstimator tensor_estimator(vm_tensor);

    ShapeTo const      new_shape_raw = shapes.second;
    ChargeAmount const expected_charge =
        VmTensorEstimator::LOW_CHARGE_CONST_COEF * fetch::vm::COMPUTE_CHARGE_COST;

    Array<SizeType>    shape_array      = GetShapeArray(new_shape_raw);
    Ptr<IArray>        new_shape_ptr    = Ptr<IArray>::PtrFromThis(&shape_array);
    ChargeAmount const estimated_charge = tensor_estimator.Reshape(new_shape_ptr);

    EXPECT_EQ(expected_charge, estimated_charge);
  }
}

TEST_F(MathTensorEstimatorTests, tensor_estimator_invalid_reshape_test)
{
  using namespace fetch::vm;
  for (ShapePair const &shapes : INVALID_TRANSFORMATIONS)
  {
    ShapeFrom const   initial_shape = shapes.first;
    MathTensor        initial_tensor{initial_shape};
    VmTensor          vm_tensor(&toolkit.vm(), TypeIds::Unknown, initial_tensor);
    VmTensorEstimator tensor_estimator(vm_tensor);

    ShapeTo const      new_shape_raw   = shapes.second;
    ChargeAmount const expected_charge = fetch::vm::MAXIMUM_CHARGE;

    Array<SizeType>    shape_array      = GetShapeArray(new_shape_raw);
    Ptr<IArray>        new_shape_ptr    = Ptr<IArray>::PtrFromThis(&shape_array);
    ChargeAmount const estimated_charge = tensor_estimator.Reshape(new_shape_ptr);

    EXPECT_EQ(expected_charge, estimated_charge);
  }
}

}  // namespace
