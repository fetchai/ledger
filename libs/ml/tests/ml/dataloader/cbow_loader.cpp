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

#include "core/fixed_point/fixed_point.hpp"
#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/word2vec_loaders/cbow_dataloader.hpp"
#include <gtest/gtest.h>
#include <string>

using namespace fetch::ml;
using namespace fetch::ml::dataloaders;

template <typename T>
CBoWTextParams<T> SetParams()
{
  using SizeType = typename T::SizeType;

  CBoWTextParams<T> ret;

  ret.n_data_buffers   = SizeType(2);  // input and context buffers
  ret.max_sentences    = SizeType(2);  // maximum number of sentences to use
  ret.discard_frequent = false;        // discard most frqeuent words
  ret.window_size      = SizeType(1);  // max size of context window one way

  return ret;
}

template <typename T>
class CBoWDataloaderTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(CBoWDataloaderTest, MyTypes);

TYPED_TEST(CBoWDataloaderTest, loader_test)
{

  std::string training_data =
      "This is a test sentence of total length ten words. This is another test sentence of total "
      "length ten words.";

  using SizeType              = typename TypeParam::SizeType;
  CBoWTextParams<TypeParam> p = SetParams<TypeParam>();
  CBoWLoader<TypeParam>     loader(p);
  loader.AddData(training_data);

  std::vector<std::pair<std::string, std::string>> gt_left_right_pairs(
      {std::pair<std::string, std::string>("this", "a"),
       std::pair<std::string, std::string>("is", "test"),
       std::pair<std::string, std::string>("a", "sentence"),
       std::pair<std::string, std::string>("test", "of"),
       std::pair<std::string, std::string>("sentence", "total"),
       std::pair<std::string, std::string>("of", "length"),
       std::pair<std::string, std::string>("total", "ten"),
       std::pair<std::string, std::string>("length", "words"),
       std::pair<std::string, std::string>("this", "another"),
       std::pair<std::string, std::string>("is", "test"),
       std::pair<std::string, std::string>("another", "sentence")});

  TypeParam left_and_right;
  for (std::size_t j = 0; j < 1000; ++j)
  {
    left_and_right = loader.GetNext().first;

    std::string left  = loader.VocabLookup(SizeType(double(left_and_right.At(0))));
    std::string right = loader.VocabLookup(SizeType(double(left_and_right.At(1))));

    ASSERT_TRUE(std::find(gt_left_right_pairs.begin(), gt_left_right_pairs.end(),
                          std::pair<std::string, std::string>(left, right)) !=
                gt_left_right_pairs.end());
  }
}
