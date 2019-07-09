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

#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <string>

using namespace fetch::ml;
using namespace fetch::ml::dataloaders;

using SizeType  = fetch::math::SizeType;
using ArrayType = fetch::math::Tensor<double>;

struct TrainingParams
{
  SizeType max_word_count       = 9;     // maximum number to be trained
  SizeType negative_sample_size = 0;     // number of negative sample per word-context pair
  SizeType window_size          = 1;     // window size for context sampling
  bool     train_mode           = true;  // reserve for future compatibility with CBOW
  double   freq_thresh          = 1;     // frequency threshold for subsampling
  SizeType min_count            = 0;     // infrequent word removal threshold
} tp;

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
  std::string training_data = "This is a test sentence of total length ten words.";

  GraphW2VLoader<double> loader(tp.window_size, tp.negative_sample_size, tp.freq_thresh,
                                tp.max_word_count, tp.train_mode);
  loader.BuildVocab({training_data});

  std::vector<std::pair<std::string, std::string>> gt_input_context_pairs(
      {std::pair<std::string, std::string>("is", "this"),
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
       std::pair<std::string, std::string>("length", "ten")});

  // test that get next works when called individually
  for (std::size_t j = 0; j < 100; ++j)
  {
    if (loader.IsDone())
    {
      loader.Reset();
    }
    std::vector<ArrayType> left_and_right = loader.GetNext().second;
    std::string input = loader.WordFromIndex(static_cast<SizeType>(left_and_right.at(0).At(0, 0)));
    std::string context =
        loader.WordFromIndex(static_cast<SizeType>(left_and_right.at(1).At(0, 0)));
    auto input_context = std::make_pair(input, context);

    ASSERT_EQ(input_context, gt_input_context_pairs.at(j % gt_input_context_pairs.size()));
  }

  // test when preparebatch is called
  bool is_done_set = false;
  auto batch       = loader.PrepareBatch(50, is_done_set).second;
  for (std::size_t j = 0; j < 50; j++)
  {
    std::string input         = loader.WordFromIndex(static_cast<SizeType>(batch.at(0).At(0, j)));
    std::string context       = loader.WordFromIndex(static_cast<SizeType>(batch.at(1).At(0, j)));
    auto        input_context = std::make_pair(input, context);

    ASSERT_EQ(input_context, gt_input_context_pairs.at(j % gt_input_context_pairs.size()));
  }
  ASSERT_EQ(is_done_set, true);
}
