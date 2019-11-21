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

#include "core/serializers/main_serializer_definition.hpp"
#include "gtest/gtest.h"
#include "ml/ops/metrics/categorical_accuracy.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include <memory>

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class CategoricalAccuracyTest : public ::testing::Test
{
};

TYPED_TEST_CASE(CategoricalAccuracyTest, math::test::HighPrecisionTensorFloatingTypes);

TYPED_TEST(CategoricalAccuracyTest, perfect_match_forward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;

  math::SizeType n_classes     = 4;
  math::SizeType n_data_points = 8;

  TensorType data1({n_classes, n_data_points});
  TensorType data2({n_classes, n_data_points});

  std::vector<math::SizeType> data = {1, 2, 3, 0, 3, 1, 0, 2};

  for (math::SizeType i = 0; i < n_data_points; ++i)
  {
    for (math::SizeType j = 0; j < n_classes; ++j)
    {
      if (data[i] == j)
      {
        data1.Set(j, i, DataType{1});
        data2.Set(j, i, DataType{1});
      }
      else
      {
        data1.Set(j, i, DataType{0});
        data2.Set(j, i, DataType{0});
      }
    }
  }

  fetch::ml::ops::CategoricalAccuracy<TensorType> op;
  TensorType                                      result({1, 1});
  op.Forward({std::make_shared<TensorType>(data1), std::make_shared<TensorType>(data2)}, result);

  EXPECT_EQ(result(0, 0), DataType{1});
}

TYPED_TEST(CategoricalAccuracyTest, mixed_forward_test)
{
  using TensorType             = TypeParam;
  using DataType               = typename TensorType::Type;
  math::SizeType n_classes     = 3;
  math::SizeType n_data_points = 2;

  TensorType data1({n_classes, n_data_points});
  TensorType data2({n_classes, n_data_points});

  data1 = TensorType::FromString("0.05, 0.9, 0.05; 0.3, 0.3, 0.4");
  data1 = data1.Transpose();
  data2 = TensorType::FromString("0, 1, 0; 1, 0, 0");
  data2 = data2.Transpose();

  fetch::ml::ops::CategoricalAccuracy<TensorType> op;
  TensorType                                      result({1, 1});
  op.Forward({std::make_shared<TensorType>(data1), std::make_shared<TensorType>(data2)}, result);

  EXPECT_NEAR(static_cast<double>(result(0, 0)), 0.5,
              static_cast<double>(fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(CategoricalAccuracyTest, mixed_forward_test_weighted)
{
  using TensorType             = TypeParam;
  using DataType               = typename TensorType::Type;
  math::SizeType n_classes     = 3;
  math::SizeType n_data_points = 2;

  TensorType data1({n_classes, n_data_points});
  TensorType data2({n_classes, n_data_points});

  data1 = TensorType::FromString("0.05, 0.9, 0.05; 0.3, 0.3, 0.4");
  data1 = data1.Transpose();
  data2 = TensorType::FromString("0, 1, 0; 1, 0, 0");
  data2 = data2.Transpose();

  TensorType weights_vector = TensorType::FromString("0.3, 0.7");
  weights_vector.Reshape({n_data_points});

  fetch::ml::ops::CategoricalAccuracy<TensorType> op(weights_vector);
  TensorType                                      result({1, 1});
  op.Forward({std::make_shared<TensorType>(data1), std::make_shared<TensorType>(data2)}, result);

  EXPECT_NEAR(static_cast<double>(result(0, 0)), 0.3,
              static_cast<double>(fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(CategoricalAccuracyTest, backward_test)
{
  using TensorType = TypeParam;

  math::SizeType n_classes     = 5;
  math::SizeType n_data_points = 7;

  TensorType data1({n_classes, n_data_points});
  TensorType data2({n_classes, n_data_points});
  TensorType error_signal({1, 1});

  fetch::ml::ops::CategoricalAccuracy<TensorType> op;

  EXPECT_THROW(
      op.Backward({std::make_shared<TensorType>(data1), std::make_shared<TensorType>(data2)},
                  error_signal),
      fetch::ml::exceptions::NotImplemented);
}

TYPED_TEST(CategoricalAccuracyTest, saveparams_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;
  using SPType     = typename fetch::ml::ops::CategoricalAccuracy<TensorType>::SPType;
  using OpType     = fetch::ml::ops::CategoricalAccuracy<TensorType>;

  math::SizeType n_classes     = 4;
  math::SizeType n_data_points = 8;

  TensorType data1({n_classes, n_data_points});
  TensorType data2({n_classes, n_data_points});

  // set gt data
  std::vector<math::SizeType> gt_data = {1, 2, 3, 0, 3, 1, 0, 2};
  for (math::SizeType i = 0; i < n_data_points; ++i)
  {
    for (math::SizeType j = 0; j < n_classes; ++j)
    {
      if (gt_data[i] == j)
      {
        data2.Set(j, i, DataType(1));
      }
      else
      {
        data2.Set(j, i, DataType(0));
      }
    }
  }

  // set softmax probabilities
  std::vector<double> logits{0.1, 0.8, 0.05, 0.05, 0.2, 0.5, 0.2, 0.1, 0.05, 0.05, 0.8,
                             0.1, 0.5, 0.1,  0.1,  0.3, 0.2, 0.3, 0.1, 0.4,  0.1,  0.7,
                             0.1, 0.1, 0.7,  0.1,  0.1, 0.1, 0.1, 0.1, 0.5,  0.3};

  math::SizeType counter{0};
  for (math::SizeType i{0}; i < n_data_points; ++i)
  {
    for (math::SizeType j{0}; j < n_classes; ++j)
    {
      data1.Set(j, i, DataType(logits[counter]));
      ++counter;
    }
  }

  OpType     op;
  TensorType result({1, 1});
  op.Forward({std::make_shared<TensorType>(data1), std::make_shared<TensorType>(data2)}, result);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original graph
  op.Forward({std::make_shared<TensorType>(data1), std::make_shared<TensorType>(data2)}, result);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_result({1, 1});
  op.Forward({std::make_shared<TensorType>(data1), std::make_shared<TensorType>(data2)},
             new_result);

  // test correct values
  EXPECT_NEAR(static_cast<double>(result(0, 0)), static_cast<double>(new_result(0, 0)),
              static_cast<double>(0));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
