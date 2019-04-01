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
#include "ml/dataloaders/word2vec_loaders/basic_textloader.hpp"
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

TYPED_TEST(TextDataLoaderTest, empty_loader_test)
{
  TextParams<TypeParam>      p;
  BasicTextLoader<TypeParam> loader(p);
  EXPECT_EQ(loader.Size(), 0);
  EXPECT_EQ(loader.VocabSize(), 0);
  EXPECT_TRUE(loader.IsDone());
}

TYPED_TEST(TextDataLoaderTest, add_data_loader_test)
{
  TextParams<TypeParam> p;
  p.full_window   = true;  //  require full window either side
  p.max_sentences = 1;
  p.window_size   = 2;

  BasicTextLoader<TypeParam> loader(p);

  EXPECT_FALSE(loader.AddData("Hello world."));
  // With a window size of 2, and full_window set to true, we need sentences of at least 5 words (2
  // + 1 + 2) to actually train We currently don't have any, hence the size == 0
  EXPECT_EQ(loader.Size(), 0);
  EXPECT_EQ(loader.VocabSize(), 0);
  EXPECT_TRUE(loader.IsDone());
  EXPECT_EQ(loader.GetVocab().size(), 0);

  EXPECT_TRUE(loader.AddData("A longer, five-word sentence"));
  EXPECT_EQ(loader.Size(), 1);       // indicates total number of valid words in training corpus
  EXPECT_EQ(loader.VocabSize(), 5);  // indicates number of unique words
  EXPECT_FALSE(loader.IsDone());
  EXPECT_EQ(loader.GetVocab().size(), 5);

  EXPECT_EQ(loader.VocabLookup(0), "a");
  EXPECT_EQ(loader.VocabLookup(1), "longer");
  EXPECT_EQ(loader.VocabLookup(2), "five");
  EXPECT_EQ(loader.VocabLookup(3), "word");
  EXPECT_EQ(loader.VocabLookup(4), "sentence");

  p.full_window = false;  //  new data loader does not require full window either side
  BasicTextLoader<TypeParam> new_loader(p);

  EXPECT_TRUE(new_loader.AddData("Hello world."));
  // Without requiring full windows, we need sentences of at least length 2 words
  EXPECT_EQ(new_loader.Size(), 2);
  EXPECT_EQ(new_loader.VocabSize(), 2);
  EXPECT_FALSE(new_loader.IsDone());
  EXPECT_EQ(new_loader.GetVocab().size(), 2);
}

TYPED_TEST(TextDataLoaderTest, loader_test)
{
  using SizeType = typename TypeParam::SizeType;

  TextParams<TypeParam> p;
  p.full_window   = true;  //  require full window either side
  p.max_sentences = 1;
  p.window_size   = 4;

  BasicTextLoader<TypeParam> loader(p);

  // check numerals not added
  EXPECT_FALSE(loader.AddData("1 78 9 1324. 57-2, 15"));
  EXPECT_EQ(loader.Size(), 0);       // only one valid position centered on I in the middle sentence
  EXPECT_EQ(loader.VocabSize(), 0);  // only middle sentence counts
  EXPECT_TRUE(loader.IsDone());
  EXPECT_EQ(loader.GetVocab().size(), 0);  // 9

  // check minimum sentence length
  EXPECT_TRUE(loader.AddData(
      "Hello, World! My name is FetchBot, I am one year old. I eat tokens for breakfast."));
  EXPECT_EQ(loader.Size(), 1);       // only one valid position centered on I in the middle sentence
  EXPECT_EQ(loader.VocabSize(), 9);  // only middle sentence counts
  EXPECT_FALSE(loader.IsDone());
  EXPECT_EQ(loader.GetVocab().size(), 9);  // 9

  // words excluded due to minimum sentence limit
  EXPECT_EQ(loader.VocabLookup("hello"), std::numeric_limits<SizeType>::max());
  EXPECT_EQ(loader.VocabLookup("world"), std::numeric_limits<SizeType>::max());
  EXPECT_EQ(loader.VocabLookup("eat"), std::numeric_limits<SizeType>::max());
  EXPECT_EQ(loader.VocabLookup("tokens"), std::numeric_limits<SizeType>::max());
  EXPECT_EQ(loader.VocabLookup("for"), std::numeric_limits<SizeType>::max());
  EXPECT_EQ(loader.VocabLookup("breakfast"), std::numeric_limits<SizeType>::max());

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

  // data check - basic text loader just returns the target word
  std::pair<TypeParam, SizeType> data;
  EXPECT_FALSE(loader.IsDone());
  data = loader.GetNext();
  EXPECT_EQ(data.first.size(), 1);  // target word -> size 1
  EXPECT_EQ(data.first.At(0), 4);   // I
  EXPECT_EQ(data.second, 1);        // irrelevant label
}

TYPED_TEST(TextDataLoaderTest, basic_loader_cycle_test)
{
  std::string training_data = "This is a test sentence of total length ten words.";

  using SizeType = typename TypeParam::SizeType;

  TextParams<TypeParam> p;
  p.max_sentences = 1;
  p.window_size   = 1;

  BasicTextLoader<TypeParam> loader(p);
  loader.AddData(training_data);

  std::vector<std::string> gt_input(
      {"this", "is", "a", "test", "sentence", "of", "total", "length", "ten", "words",
       "this", "is", "a", "test", "sentence", "of", "total", "length", "ten", "words"});

  std::string cur_word;
  for (std::size_t j = 0; j < 20; ++j)
  {
    if (loader.IsDone())
    {
      loader.Reset();
    }
    std::pair<TypeParam, SizeType> output = loader.GetNext();
    cur_word = loader.VocabLookup(SizeType(double(output.first.At(0))));

    ASSERT_EQ(cur_word, gt_input.at(j));
  }
}

TYPED_TEST(TextDataLoaderTest, adddata_loader_test)
{
  std::string training_data = "This is a test sentence of total length ten words.";

  using SizeType = typename TypeParam::SizeType;

  TextParams<TypeParam> p;
  p.max_sentences = 2;
  p.window_size   = 1;

  BasicTextLoader<TypeParam> loader(p);
  loader.AddData(training_data);

  std::vector<std::string> gt_input(
      {"this", "is", "a", "test", "sentence", "of", "total", "length", "ten", "words",
       "this", "is", "a", "test", "sentence", "of", "total", "length", "ten", "words"});

  std::string cur_word;
  for (std::size_t j = 0; j < 20; ++j)
  {
    if (loader.IsDone())
    {
      loader.Reset();
    }
    std::pair<TypeParam, SizeType> output = loader.GetNext();
    cur_word = loader.VocabLookup(SizeType(double(output.first.At(0))));
    ASSERT_TRUE(cur_word.compare(gt_input.at(j)) == 0);
  }

  std::string new_training_data = "This is a new sentence added after set up.";

  loader.AddData(new_training_data);
  loader.Reset();

  gt_input = {"this",  "is",    "a",    "test", "sentence", "of",  "total",    "length",
              "ten",   "words", "this", "is",   "a",        "new", "sentence", "added",
              "after", "set",   "up",   "this", "is",       "a"};

  for (std::size_t j = 0; j < 22; ++j)
  {
    if (loader.IsDone())
    {
      loader.Reset();
    }
    std::pair<TypeParam, SizeType> output = loader.GetNext();
    cur_word = loader.VocabLookup(SizeType(double(output.first.At(0))));
    ASSERT_TRUE(cur_word.compare(gt_input.at(j)) == 0);
  }
}

TYPED_TEST(TextDataLoaderTest, punctuation_loader_test)
{
  std::string training_data =
      "This is a test sentence of total length ten words. This next sentence doesn't make things "
      "so easy, because it has some punctuation, doesn't it? Indeed it does, and this-sentence "
      "even-has hyphenations and ends on the following quote: \"this is a quote.\" And this last "
      "sentence is ignored due to exceeding max sentences.";

  using SizeType = typename TypeParam::SizeType;

  TextParams<TypeParam> p;
  p.max_sentences = 3;
  p.window_size   = 1;

  BasicTextLoader<TypeParam> loader(p);
  loader.AddData(training_data);

  std::vector<std::string> gt_input(
      {"this",      "is",    "a",    "test",         "sentence", "of",    "total", "length",
       "ten",       "words", "this", "next",         "sentence", "doesn", "t",     "make",
       "things",    "so",    "easy", "because",      "it",       "has",   "some",  "punctuation",
       "doesn",     "t",     "it",   "indeed",       "it",       "does",  "and",   "this",
       "sentence",  "even",  "has",  "hyphenations", "and",      "ends",  "on",    "the",
       "following", "quote", "this", "is",           "a",        "quote"});

  std::string cur_word;
  for (std::size_t j = 0; j < gt_input.size(); ++j)
  {
    if (loader.IsDone())
    {
      loader.Reset();
    }
    std::pair<TypeParam, SizeType> output = loader.GetNext();
    cur_word = loader.VocabLookup(SizeType(double(output.first.At(0))));
    ASSERT_EQ(cur_word, gt_input.at(j));
  }
}

TYPED_TEST(TextDataLoaderTest, discard_loader_test)
{
  std::string training_data = "This is a test sentence of total length ten words.";

  TextParams<TypeParam> p;
  p.max_sentences = 1;
  p.window_size   = 1;

  p.discard_frequent  = true;
  p.discard_threshold = 0.000000001;  // extreme discard threshold so no words pass in this case

  BasicTextLoader<TypeParam> loader(p);
  ASSERT_TRUE(loader.AddData(training_data));
  ASSERT_EQ(loader.GetDiscardCount(), loader.VocabSize());

  // since there are no valid words, calling get next will fail leaving an unset cursor position
  // which is forbidden
  EXPECT_THROW(loader.GetNext(), std::runtime_error);

  // to demonstrate this doesn't happen without discards we repeat the experiment
  TextParams<TypeParam> p2;
  p2.max_sentences    = 1;
  p2.window_size      = 1;
  p2.discard_frequent = false;

  BasicTextLoader<TypeParam> loader2(p2);
  ASSERT_TRUE(loader2.AddData(training_data));

  std::pair<TypeParam, typename TypeParam::SizeType> output = loader2.GetNext();
}