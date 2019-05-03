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
#include <gtest/gtest.h>

using namespace std;
using namespace std::chrono;

template <typename T>
class TensorConcatenationTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<int, unsigned int, long, unsigned long, float, double>;
TYPED_TEST_CASE(TensorConcatenationTest, MyTypes);

TYPED_TEST(TensorConcatenationTest, tensor_concatenation)
{
  using Tensor = fetch::math::Tensor<TypeParam>;
  Tensor t1 = Tensor::FromString("0 1 2; 3 4 5");
  Tensor t2 = Tensor::FromString("0 1 2; 3 4 5");
  Tensor t3 = Tensor::FromString("0 1 2; 3 4 5");
  Tensor gt = Tensor::FromString("0 1 2; 3 4 5; 0 1 2; 3 4 5; 0 1 2; 3 4 5");

  std::vector<Tensor> vt{t1, t2, t3};
  Tensor ret = Tensor::Concat(vt, 0);

  EXPECT_TRUE(ret.shape() == gt.shape());

  std::cout << "gt.ToString(): " << gt.ToString() << std::endl;
  std::cout << "ret.ToString(): " << ret.ToString() << std::endl;
  
//  auto gt_it2 = gt.begin();
//  while(gt_it2.is_valid())
//  {
//    std::cout << "*gt_it2: " << *gt_it2 << std::endl;
//    ++gt_it2;
//  }
//  auto it2 = ret.begin();
//  while(it2.is_valid())
//  {
//    std::cout << "*it2: " << *it2 << std::endl;
//    ++it2;
//  }

  EXPECT_TRUE(ret.AllClose(gt));



//  using Tensor = fetch::math::Tensor<TypeParam>;
//  Tensor t1({2, 3, 4});
//  Tensor t2({2, 3, 4});
//  Tensor t3({2, 3, 4});
//  Tensor gt({2, 9, 4});
//
//  auto t_it1 = t1.begin();
//  auto t_it2 = t2.begin();
//  auto t_it3 = t3.begin();
//  auto gt_it = gt.begin();
//
//  TypeParam count_idx = 0;
//  while(t_it1.is_valid())
//  {
//    *t_it1 = count_idx;
//    *gt_it = count_idx;
//    ++t_it1;
//    ++gt_it;
//    ++count_idx;
//  }
//  count_idx = 0;
//  while(t_it2.is_valid())
//  {
//    *t_it2 = count_idx;
//    *gt_it = count_idx;
//    ++t_it2;
//    ++gt_it;
//    ++count_idx;
//  }
//  count_idx = 0;
//  while(t_it3.is_valid())
//  {
//    *t_it3 = count_idx;
//    *gt_it = count_idx;
//    ++t_it3;
//    ++gt_it;
//    ++count_idx;
//  }
//
//  std::vector<Tensor> vt{t1, t2, t3};
//  Tensor ret = Tensor::Concat(vt, 1);
//
//  EXPECT_TRUE(ret.shape() == gt.shape());
//
//  auto gt_it2 = gt.begin();
//  while(gt_it2.is_valid())
//  {
//    std::cout << "*gt_it2: " << *gt_it2 << std::endl;
//    ++gt_it2;
//  }
//
//  auto it2 = ret.begin();
//  while(it2.is_valid())
//  {
//    std::cout << "*it2: " << *it2 << std::endl;
//    ++it2;
//  }
//
//  EXPECT_TRUE(ret.AllClose(gt));
}
