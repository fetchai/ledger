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

#include "math/tensor.hpp"
#include "ml/clustering/tsne.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class TsneTests : public ::testing::Test
{
};

// we do not test for fp32 since that tends to overflow
TYPED_TEST_CASE(TsneTests, math::test::HighPrecisionTensorFloatingTypes);

template <typename TypeParam>
TypeParam RunTest(typename TypeParam::SizeType n_output_feature_size,
                  typename TypeParam::SizeType n_data_size)
{
  using SizeType   = fetch::math::SizeType;
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  SizeType RANDOM_SEED{123456};
  auto     LEARNING_RATE = DataType{500};  // (seems very high!)
  SizeType MAX_ITERATIONS{1};
  auto     PERPLEXITY = DataType{20};
  SizeType N_DATA_SIZE{n_data_size};
  SizeType N_INPUT_FEATURE_SIZE{3};
  SizeType N_OUTPUT_FEATURE_SIZE{n_output_feature_size};
  DataType INITIAL_MOMENTUM = fetch::math::Type<DataType>("0.5");
  DataType FINAL_MOMENTUM   = fetch::math::Type<DataType>("0.8");
  SizeType FINAL_MOMENTUM_STEPS{20};
  SizeType P_LATER_CORRECTION_ITERATION{10};

  TensorType A({N_INPUT_FEATURE_SIZE, N_DATA_SIZE});

  // Generate easily separable clusters of data
  for (SizeType i = 0; i < 25; ++i)
  {
    A(0, i) = -static_cast<DataType>(i) - static_cast<DataType>(50);
    A(1, i) = -static_cast<DataType>(i) - static_cast<DataType>(50);
    A(2, i) = -static_cast<DataType>(i) - static_cast<DataType>(50);
  }
  for (SizeType i = 25; i < 50; ++i)
  {
    A(0, i) = -static_cast<DataType>(i) - static_cast<DataType>(50);
    A(1, i) = static_cast<DataType>(i) + static_cast<DataType>(50);
    A(2, i) = static_cast<DataType>(i) + static_cast<DataType>(50);
  }
  for (SizeType i = 50; i < 75; ++i)
  {
    A(0, i) = static_cast<DataType>(i) + static_cast<DataType>(50);
    A(1, i) = -static_cast<DataType>(i) - static_cast<DataType>(50);
    A(2, i) = -static_cast<DataType>(i) - static_cast<DataType>(50);
  }
  for (SizeType i = 75; i < 100; ++i)
  {
    A(0, i) = static_cast<DataType>(i) + static_cast<DataType>(50);
    A(1, i) = static_cast<DataType>(i) + static_cast<DataType>(50);
    A(2, i) = static_cast<DataType>(i) + static_cast<DataType>(50);
  }

  fetch::ml::TSNE<TensorType> tsn(A, N_OUTPUT_FEATURE_SIZE, PERPLEXITY, RANDOM_SEED);

  tsn.Optimise(LEARNING_RATE, MAX_ITERATIONS, INITIAL_MOMENTUM, FINAL_MOMENTUM,
               FINAL_MOMENTUM_STEPS, P_LATER_CORRECTION_ITERATION);
  return tsn.GetOutputMatrix();
}

TYPED_TEST(TsneTests, tsne_test_2d)
{
  using DataType = typename TypeParam::Type;
  math::SizeType n_data{100};
  math::SizeType n_features{2};

  TypeParam output_matrix = RunTest<TypeParam>(n_features, n_data);

  ASSERT_EQ(output_matrix.shape().at(0), n_features);
  ASSERT_EQ(output_matrix.shape().at(1), n_data);

  // NB: next time the random number generator changes the test values will change. Use this:
  //  std::cout << "output_matrix.ToString(): " << output_matrix.ToString() << std::endl;
  // to get the new gt values.
  TypeParam gt = TypeParam::FromString(
      "+2.034727722, +2.007600134, -4.061901712, -5.061318479, +1.063344098, -2.273924887, "
      "-0.929571316, -0.120476458, +2.870792931, +4.368727957, -1.886078063, +4.091233246, "
      "-4.754639734, -1.693883608, +2.119996617, +2.283251317, +0.624434071, +2.907723532, "
      "-7.073848520, -0.742194112, +2.659493614, -2.718192698, +3.115057095, +0.384604697, "
      "-1.419326801, +1.174573460, -1.856119608, +0.629172630, -3.314039848, +3.934781696, "
      "+0.856851469, +3.864086929, +1.284692184, -4.911365864, +4.832059899, +0.045174903, "
      "-0.893270237, -5.499326173, -2.957856182, -1.548451201, -0.641953249, +3.893961627, "
      "+3.677957676, +3.245233109, +0.634536414, +0.989730516, -2.353329550, +1.353518064, "
      "+1.297092193, -0.972176393, +3.480457320, +2.171576278, -2.268467837, +2.208108676, "
      "-1.620713518, -2.593036924, +4.925395254, -3.557535635, -3.801955620, -2.529879658, "
      "-3.281961481, +3.807888131, +0.369679825, +4.711608956, -3.717754051, -3.265910859, "
      "+3.972900404, -4.681000492, +3.297463849, +1.926674014, -0.067391085, +3.074820942, "
      "+2.056793843, -0.547372613, +1.413410868, -0.129596003, -0.633182505, +1.428318425, "
      "+0.522538007, +0.008656723, +0.873033218, +2.380449050, +0.452281800, +1.857348358, "
      "-6.603170778, +3.149140904, -0.878323242, -6.018770059, -7.874319683, +1.687370152, "
      "+0.508177752, +0.519719064, +0.778411260, +0.401316901, +2.668452364, -2.564438170, "
      "-5.844760728, +0.907398018, +0.444165639, +1.914819826;-2.279339516, +0.798761774, "
      "-3.192759086, +1.479517598, +4.035884512, +2.173781130, +1.185005513, +1.808261687, "
      "+0.586591977, -1.996865659, +0.186799705, +1.133234309, -5.461735067, +2.653832192, "
      "-1.984641954, -4.348928783, -2.275644174, +4.552709739, -0.386516443, +3.459837894, "
      "-0.790932285, +3.374883652, -1.053481963, +0.092978250, -1.717628032, +2.109380653, "
      "+0.319771341, +3.086400158, -2.328460556, -2.340656643, +3.950538059, -3.336229487, "
      "+3.760451575, -3.031936624, +0.274963641, -1.921015901, -4.014275384, +4.603713542, "
      "-4.379757500, +4.192695743, -4.532183236, +3.627416426, +3.382097056, -0.773111152, "
      "+2.090842954, -2.385545611, -3.683948152, +1.183539767, -2.793507375, +3.561646909, "
      "+1.192687907, +0.377735635, +2.667860150, -4.864787100, +0.674980394, +2.847327586, "
      "+1.314147818, +1.568130487, -2.598822838, +3.172692804, -6.790442933, -4.271838205, "
      "+0.513887301, +0.863027766, -2.242399951, +1.466006449, -5.163237123, -2.208913316, "
      "+3.812654925, +0.903166944, +3.201328009, -2.000225958, -4.019561596, +0.798409645, "
      "+1.929299942, -0.507108332, +3.319433392, +3.152668119, +0.297959888, -1.572831010, "
      "-4.336589294, -2.283604071, +4.397173648, +3.544610600, +3.989913126, -0.505564599, "
      "-3.697668325, -3.426335271, -2.339205314, -4.094425885, -3.842632682, +4.129424204, "
      "+4.689516241, -3.092365617, +3.405012754, -0.397711445, +0.837367840, +0.776023158, "
      "+0.676667879, +1.080719065;");

  // in general we set the tolerance to be function tolerance * number of operations.
  // since tsne is a training procedure the number of operations is relatively large.
  // here we use 50 as proxy instead of calculating the number of operations
  // 50 is quite strict since there are 100 data points
  DataType tolerance = DataType{50} * std::max(math::function_tolerance<DataType>(),
                                               fetch::math::Type<DataType>("0.000000001"));

  EXPECT_TRUE(output_matrix.AllClose(gt, DataType{0}, tolerance));

  EXPECT_NEAR(double(output_matrix.At(0, 0)), double(gt.At(0, 0)), static_cast<double>(tolerance));
  EXPECT_NEAR(double(output_matrix.At(1, 0)), double(gt.At(1, 0)), static_cast<double>(tolerance));
  EXPECT_NEAR(double(output_matrix.At(0, 25)), double(gt.At(0, 25)),
              static_cast<double>(tolerance));
  EXPECT_NEAR(double(output_matrix.At(1, 25)), double(gt.At(1, 25)),
              static_cast<double>(tolerance));
  EXPECT_NEAR(double(output_matrix.At(0, 50)), double(gt.At(0, 50)),
              static_cast<double>(tolerance));
  EXPECT_NEAR(double(output_matrix.At(1, 50)), double(gt.At(1, 50)),
              static_cast<double>(tolerance));
  EXPECT_NEAR(double(output_matrix.At(0, 99)), double(gt.At(0, 99)),
              static_cast<double>(tolerance));
  EXPECT_NEAR(double(output_matrix.At(1, 99)), double(gt.At(1, 99)),
              static_cast<double>(tolerance));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
