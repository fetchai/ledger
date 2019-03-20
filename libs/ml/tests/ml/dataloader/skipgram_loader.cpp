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
#include "ml/dataloaders/skipgram_dataloader.hpp"
#include <gtest/gtest.h>
#include <string>

using namespace fetch::ml;
using namespace fetch::ml::dataloaders;

template <typename T>
SkipGramTextParams<T> SetParams()
{
  using SizeType = typename T::SizeType;

  SkipGramTextParams<T> ret;

  ret.n_data_buffers     = SizeType(2);  // input and context buffers
  ret.max_sentences      = SizeType(1);  // maximum number of sentences to use
  ret.unigram_table      = false;        // unigram table for sampling negative training pairs
  ret.discard_frequent   = false;        // discard most frqeuent words
  ret.window_size        = SizeType(1);  // max size of context window one way
  ret.k_negative_samples = SizeType(0);  // number of negative examples to sample

  return ret;
}

template <typename T>
class SkipGramDataloaderTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(SkipGramDataloaderTest, MyTypes);

TYPED_TEST(SkipGramDataloaderTest, loader_test)
{

  std::string TRAINING_DATA =
      "This is a test sentence of total length ten words. This is another test sentence of total "
      "length ten words.";

  using SizeType                  = typename TypeParam::SizeType;
  SkipGramTextParams<TypeParam> p = SetParams<TypeParam>();
  SkipGramLoader<TypeParam>     loader(TRAINING_DATA, p);

  std::vector<std::pair<std::string, std::string>> gt_input_context_pairs(
      {std::pair<std::string, std::string>("this", "is"),
       std::pair<std::string, std::string>("is", "this"),
       std::pair<std::string, std::string>("is", "a"),
       std::pair<std::string, std::string>("a", "is"),
       std::pair<std::string, std::string>("a", "test"),
       std::pair<std::string, std::string>("test", "a"),
       std::pair<std::string, std::string>("test", "sentence"),
       std::pair<std::string, std::string>("sentence", "test"),
       std::pair<std::string, std::string>("sentence", "of"),
       std::pair<std::string, std::string>("of", "sentence"),
       std::pair<std::string, std::string>("of", "total"),
       std::pair<std::string, std::string>("total", "of"),
       std::pair<std::string, std::string>("total", "length"),
       std::pair<std::string, std::string>("length", "total"),
       std::pair<std::string, std::string>("length", "ten"),
       std::pair<std::string, std::string>("ten", "length"),
       std::pair<std::string, std::string>("ten", "words"),
       std::pair<std::string, std::string>("words", "ten"),
       std::pair<std::string, std::string>("is", "another"),
       std::pair<std::string, std::string>("another", "is"),
       std::pair<std::string, std::string>("another", "test"),
       std::pair<std::string, std::string>("test", "another")});

  TypeParam input_and_context;
  for (std::size_t j = 0; j < 1000; ++j)
  {
    input_and_context = loader.GetNext().first;

    std::string input   = loader.VocabLookup(SizeType(double(input_and_context.At(0))));
    std::string context = loader.VocabLookup(SizeType(double(input_and_context.At(1))));

    ASSERT_TRUE(std::find(gt_input_context_pairs.begin(), gt_input_context_pairs.end(),
                          std::pair<std::string, std::string>(input, context)) !=
                gt_input_context_pairs.end());
  }
}
