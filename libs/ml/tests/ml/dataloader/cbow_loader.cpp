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
#include "math/matrix_operations.hpp"
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

  ret.n_data_buffers   = SizeType(2);  // just one value either side of target word
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

TYPED_TEST(CBoWDataloaderTest, small_window_loader_test)
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
       std::pair<std::string, std::string>("another", "sentence"),
       std::pair<std::string, std::string>("test", "of"),
       std::pair<std::string, std::string>("sentence", "total"),
       std::pair<std::string, std::string>("of", "length"),
       std::pair<std::string, std::string>("total", "ten"),
       std::pair<std::string, std::string>("length", "words")});

  TypeParam left_and_right;
  for (std::size_t j = 0; j < 100; ++j)
  {
    if (loader.IsDone())
    {
      loader.Reset();
    }

    left_and_right = loader.GetNext().first;

    std::string left  = loader.VocabLookup(SizeType(double(left_and_right.At(0))));
    std::string right = loader.VocabLookup(SizeType(double(left_and_right.At(1))));

    ASSERT_EQ(std::make_pair(left, right), gt_left_right_pairs.at(j % gt_left_right_pairs.size()));
  }
}

TYPED_TEST(CBoWDataloaderTest, large_window_loader_test)
{
  using SizeType = typename TypeParam::SizeType;

  CBoWTextParams<TypeParam> p;
  p.max_sentences  = 1;
  p.window_size    = 4;
  p.n_data_buffers = p.window_size * 2;

  CBoWLoader<TypeParam> loader(p);

  // check numerals not added
  EXPECT_FALSE(loader.AddData("1 78 9 1324. 57-2, 15"));
  EXPECT_EQ(loader.Size(), 0);
  EXPECT_EQ(loader.VocabSize(), 0);
  EXPECT_TRUE(loader.IsDone());
  EXPECT_EQ(loader.GetVocab().size(), 0);  // 9

  // check minimum sentence length
  EXPECT_TRUE(loader.AddData(
      "Hello, World! My name is FetchBot, I am one year old and I eat tokens for breakfast."));
  EXPECT_EQ(loader.Size(), 7);
  EXPECT_EQ(loader.VocabSize(), 14);
  EXPECT_FALSE(loader.IsDone());
  EXPECT_EQ(loader.GetVocab().size(), 14);  // 9

  // words excluded due to minimum sentence limit
  EXPECT_EQ(loader.VocabLookup("hello"), std::numeric_limits<SizeType>::max());
  EXPECT_EQ(loader.VocabLookup("world"), std::numeric_limits<SizeType>::max());

  // words included
  EXPECT_EQ(loader.VocabLookup("my"), SizeType(0));
  EXPECT_EQ(loader.VocabLookup("name"), SizeType(1));
  EXPECT_EQ(loader.VocabLookup("is"), SizeType(2));
  EXPECT_EQ(loader.VocabLookup("fetchbot"), SizeType(3));
  EXPECT_EQ(loader.VocabLookup("i"), SizeType(4));
  EXPECT_EQ(loader.VocabLookup("am"), SizeType(5));
  EXPECT_EQ(loader.VocabLookup("one"), SizeType(6));
  EXPECT_EQ(loader.VocabLookup("year"), SizeType(7));
  EXPECT_EQ(loader.VocabLookup("old"), SizeType(8));

  // check queries with variable casing and punctuation
  EXPECT_EQ(loader.VocabLookup("My"), SizeType(0));
  EXPECT_EQ(loader.VocabLookup("Name"), SizeType(1));
  EXPECT_EQ(loader.VocabLookup("iS"), SizeType(2));
  EXPECT_EQ(loader.VocabLookup("FetchBot"), SizeType(3));
  EXPECT_EQ(loader.VocabLookup("I"), SizeType(4));
  EXPECT_EQ(loader.VocabLookup("am"), SizeType(5));
  EXPECT_EQ(loader.VocabLookup("onE"), SizeType(6));
  EXPECT_EQ(loader.VocabLookup("yEar"), SizeType(7));
  EXPECT_EQ(loader.VocabLookup("oLd"), SizeType(8));

  // check bad word lookups
  EXPECT_EQ(loader.VocabLookup("multi-word-lookup"), std::numeric_limits<SizeType>::max());
  EXPECT_EQ(loader.VocabLookup("$£%^*($"), std::numeric_limits<SizeType>::max());

  // data check
  std::pair<TypeParam, SizeType> data;
  EXPECT_FALSE(loader.IsDone());
  data = loader.GetNext();
  EXPECT_EQ(data.first.size(), p.n_data_buffers);  //
  EXPECT_EQ(data.first.At(0), 0);                  // my
  EXPECT_EQ(data.first.At(1), 1);                  // name
  EXPECT_EQ(data.first.At(2), 2);                  // is
  EXPECT_EQ(data.first.At(3), 3);                  // fetchbot
  EXPECT_EQ(data.second, 4);                       // I
  EXPECT_EQ(data.first.At(4), 5);                  // am
  EXPECT_EQ(data.first.At(5), 6);                  // one
  EXPECT_EQ(data.first.At(6), 7);                  // year
  EXPECT_EQ(data.first.At(7), 8);                  // old

  EXPECT_FALSE(loader.IsDone());
  data = loader.GetNext();
  EXPECT_EQ(data.first.size(), p.n_data_buffers);  // 8 = 2 * windowSize
  EXPECT_EQ(data.first.At(0), 1);                  // name
  EXPECT_EQ(data.first.At(1), 2);                  // is
  EXPECT_EQ(data.first.At(2), 3);                  // fetchbot
  EXPECT_EQ(data.first.At(3), 4);                  // I
  EXPECT_EQ(data.second, 5);                       // am
  EXPECT_EQ(data.first.At(4), 6);                  // one
  EXPECT_EQ(data.first.At(5), 7);                  // year
  EXPECT_EQ(data.first.At(6), 8);                  // old
  EXPECT_EQ(data.first.At(7), 9);                  // and

  EXPECT_FALSE(loader.IsDone());
  data = loader.GetNext();
  EXPECT_EQ(data.first.size(), p.n_data_buffers);  // 8 = 2 * windowSize
  EXPECT_EQ(data.first.At(0), 2);                  // is
  EXPECT_EQ(data.first.At(1), 3);                  // fetchbot
  EXPECT_EQ(data.first.At(2), 4);                  // I
  EXPECT_EQ(data.first.At(3), 5);                  // am
  EXPECT_EQ(data.second, 6);                       // one
  EXPECT_EQ(data.first.At(4), 7);                  // year
  EXPECT_EQ(data.first.At(5), 8);                  // old
  EXPECT_EQ(data.first.At(6), 9);                  // and
  EXPECT_EQ(data.first.At(7), 4);                  // I

  EXPECT_FALSE(loader.IsDone());
  data = loader.GetNext();  // fetchbot I am one year old and I eat
  EXPECT_FALSE(loader.IsDone());
  data = loader.GetNext();  // I am one year old and I eat tokens
  EXPECT_FALSE(loader.IsDone());
  data = loader.GetNext();  // am one year old and I eat tokens for
  EXPECT_FALSE(loader.IsDone());
  data = loader.GetNext();  // year old and I eat tokens for breakfast
  EXPECT_TRUE(loader.IsDone());
}
