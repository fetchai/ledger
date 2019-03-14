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

std::string TRAINING_DATA = "This is a test sentence of total length ten words.";

template <typename T>
SkipGramTextParams<T> SetParams()
{
  using SizeType = typename T::SizeType;

  SkipGramTextParams<T> ret;

  ret.n_data_buffers     = SizeType(2);  // input and context buffers
  ret.max_sentences      = SizeType(1);  // maximum number of sentences to use
  ret.unigram_table      = false;        // unigram table for sampling negative training pairs
  ret.discard_frequent   = false;        // discard most frqeuent words
  ret.skip_window        = SizeType(1);  // max size of context window one way
  ret.super_sampling     = SizeType(1);  // n times to reuse an input to generate a label
  ret.k_negative_samples = SizeType(0);  // number of negative examples to sample

  return ret;
}

template <typename TypeParam>
std::pair<std::string, std::string> GetStrings(SkipGramLoader<TypeParam> loader,
                                               TypeParam                 input_and_context_one_hot)
{
  using SizeType = typename TypeParam::SizeType;

  TypeParam input({1, loader.VocabSize()});
  TypeParam context({1, loader.VocabSize()});
  SizeType  idx = 0;
  for (auto &e : input_and_context_one_hot)
  {
    if (idx < loader.VocabSize())
    {
      input[idx] = e;
    }
    else
    {
      context[idx - loader.VocabSize()] = e;
    }
    idx++;
  }

  std::string input_str   = loader.VocabLookup(SizeType(double(fetch::math::ArgMax(input))));
  std::string context_str = loader.VocabLookup(SizeType(double(fetch::math::ArgMax(context))));

  return std::make_pair(input_str, context_str);
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
  //  using Type = typename TypeParam::Type;
  //  using SizeType = typename TypeParam::SizeType;

  SkipGramTextParams<TypeParam> p = SetParams<TypeParam>();
  SkipGramLoader<TypeParam>     loader(TRAINING_DATA, p);

  std::vector<std::string> gt_input({"this", "is", "is", "a", "a", "test", "test", "sentence",
                                     "sentence", "of", "of", "total", "total", "length", "length",
                                     "ten", "ten", "words"});
  std::vector<std::string> gt_context({"is", "this", "a", "is", "test", "a", "sentence", "test",
                                       "of", "sentence", "total", "of", "length", "total", "ten",
                                       "length", "words", "ten"});

  std::pair<std::string, std::string> input_and_context;
  for (std::size_t j = 0; j < gt_input.size(); ++j)
  {
    input_and_context = GetStrings(loader, *loader.GetNext().first);
    ASSERT_TRUE(input_and_context.first.compare(gt_input[j]) == 0);
    ASSERT_TRUE(input_and_context.second.compare(gt_context[j]) == 0);
  }
}
