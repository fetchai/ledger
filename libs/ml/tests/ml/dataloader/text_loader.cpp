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

#include "ml/dataloaders/text_loader.hpp"
#include "core/fixed_point/fixed_point.hpp"
#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>
#include <string>

using namespace fetch::ml;
using namespace fetch::ml::dataloaders;

template <typename T>
class TextDataLoaderTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(TextDataLoaderTest, MyTypes);

TYPED_TEST(TextDataLoaderTest, loader_test)
{
  std::string TRAINING_DATA = "This is a test sentence of total length ten words.";

  using SizeType = typename TypeParam::SizeType;

  TextParams<TypeParam> p;
  p.n_data_buffers      = 1;
  p.max_sentences       = 1;
  p.min_sentence_length = 0;
  p.window_size         = 1;
  p.unigram_table       = false;
  p.discard_frequent    = false;

  TextLoader<TypeParam> loader(TRAINING_DATA, p);

  std::vector<std::string> gt_input(
      {"this", "is", "a", "test", "sentence", "of", "total", "length", "ten", "words",
       "this", "is", "a", "test", "sentence", "of", "total", "length", "ten", "words"});

  std::string cur_word;
  for (std::size_t j = 0; j < 20; ++j)
  {
    std::pair<TypeParam, SizeType> output = loader.GetNext();
    cur_word                              = loader.VocabLookup(SizeType(output.first.At(0)));
    ASSERT_TRUE(cur_word.compare(gt_input.at(j)) == 0);
  }
}
