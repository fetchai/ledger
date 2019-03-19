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

#include "ml/dataloaders/w2v_cbow_dataloader.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

TEST(CBOWDataloaderTest, empty_loader_test)
{
  fetch::ml::CBOWLoader<float> loader(4);
  EXPECT_EQ(loader.Size(), 0);
  EXPECT_EQ(loader.VocabSize(), 0);
  EXPECT_TRUE(loader.IsDone());
}

TEST(CBOWDataloaderTest, add_data_loader_test)
{
  fetch::ml::CBOWLoader<float> loader(1);
  EXPECT_FALSE(loader.AddData("Hello World"));
  // With a window size of 4, we need sentences of at least 9 words (4 + 1 + 4) to actually train
  // We currently don't have any, hence the size == 0
  EXPECT_EQ(loader.Size(), 0);
  EXPECT_EQ(loader.VocabSize(), 0);
  EXPECT_TRUE(loader.IsDone());
  EXPECT_EQ(loader.GetVocab().size(), 0);

  EXPECT_TRUE(loader.AddData("Open Economic Framework"));
  EXPECT_EQ(loader.Size(), 1);
  EXPECT_EQ(loader.VocabSize(), 3);
  EXPECT_FALSE(loader.IsDone());
  EXPECT_EQ(loader.GetVocab().size(), 3);
  EXPECT_EQ(loader.GetVocab().at("open"), 0);
  EXPECT_EQ(loader.GetVocab().at("economic"), 1);
  EXPECT_EQ(loader.GetVocab().at("framework"), 2);
}

TEST(CBOWDataloaderTest, loader_test)
{
  fetch::ml::CBOWLoader<float> loader(4);
  EXPECT_TRUE(loader.AddData(
      "Hello, World! My name is FetchBot, I am 1 year old. I eat tokens for breakfast."));
  EXPECT_EQ(loader.Size(), 7);
  EXPECT_EQ(loader.VocabSize(), 14);
  EXPECT_FALSE(loader.IsDone());
  EXPECT_EQ(loader.GetVocab().size(), 14);

  EXPECT_EQ(loader.GetVocab().at("hello"), 0);
  EXPECT_EQ(loader.GetVocab().at("world"), 1);
  EXPECT_EQ(loader.GetVocab().at("my"), 2);
  EXPECT_EQ(loader.GetVocab().at("name"), 3);
  EXPECT_EQ(loader.GetVocab().at("is"), 4);
  EXPECT_EQ(loader.GetVocab().at("fetchbot"), 5);
  EXPECT_EQ(loader.GetVocab().at("i"), 6);
  EXPECT_EQ(loader.GetVocab().at("am"), 7);
  EXPECT_THROW(loader.GetVocab().at("1"), std::out_of_range);
  EXPECT_EQ(loader.GetVocab().at("year"), 8);
  EXPECT_EQ(loader.GetVocab().at("old"), 9);
  EXPECT_EQ(loader.GetVocab().at("eat"), 10);
  EXPECT_EQ(loader.GetVocab().at("tokens"), 11);
  EXPECT_EQ(loader.GetVocab().at("for"), 12);
  EXPECT_EQ(loader.GetVocab().at("breakfast"), 13);

  std::pair<fetch::math::Tensor<float>, uint64_t> data;

  EXPECT_FALSE(loader.IsDone());
  data = loader.GetNext();
  EXPECT_EQ(data.first.size(), 8);  // 8 = 2 * windowSize
  EXPECT_EQ(data.first.At(0), 0);   // hello
  EXPECT_EQ(data.first.At(1), 1);   // world
  EXPECT_EQ(data.first.At(2), 2);   // my
  EXPECT_EQ(data.first.At(3), 3);   // name
  EXPECT_EQ(data.second, 4);        // is
  EXPECT_EQ(data.first.At(4), 5);   // fetchbot
  EXPECT_EQ(data.first.At(5), 6);   // i
  EXPECT_EQ(data.first.At(6), 7);   // am
  EXPECT_EQ(data.first.At(7), 8);   // year

  EXPECT_FALSE(loader.IsDone());
  data = loader.GetNext();
  EXPECT_EQ(data.first.size(), 8);  // 8 = 2 * windowSize
  EXPECT_EQ(data.first.At(0), 1);   // world
  EXPECT_EQ(data.first.At(1), 2);   // my
  EXPECT_EQ(data.first.At(2), 3);   // name
  EXPECT_EQ(data.first.At(3), 4);   // is
  EXPECT_EQ(data.second, 5);        // fetchbot
  EXPECT_EQ(data.first.At(4), 6);   // i
  EXPECT_EQ(data.first.At(5), 7);   // am
  EXPECT_EQ(data.first.At(6), 8);   // year
  EXPECT_EQ(data.first.At(7), 9);   // old

  EXPECT_FALSE(loader.IsDone());
  data = loader.GetNext();
  EXPECT_EQ(data.first.size(), 8);  // 8 = 2 * windowSize
  EXPECT_EQ(data.first.At(0), 2);   // my
  EXPECT_EQ(data.first.At(1), 3);   // name
  EXPECT_EQ(data.first.At(2), 4);   // is
  EXPECT_EQ(data.first.At(3), 5);   // fetchbot
  EXPECT_EQ(data.second, 6);        // i
  EXPECT_EQ(data.first.At(4), 7);   // am
  EXPECT_EQ(data.first.At(5), 8);   // year
  EXPECT_EQ(data.first.At(6), 9);   // old
  EXPECT_EQ(data.first.At(7), 6);   // I

  EXPECT_FALSE(loader.IsDone());
  data = loader.GetNext();  // name is FetchBot, I am year old. I eat
  EXPECT_FALSE(loader.IsDone());
  data = loader.GetNext();  // is FetchBot, I am year old. I eat tokens
  EXPECT_FALSE(loader.IsDone());
  data = loader.GetNext();  // FetchBot, I am year old. I eat tokens for
  EXPECT_FALSE(loader.IsDone());
  data = loader.GetNext();  // I am year old. I eat tokens for breakfast
  EXPECT_TRUE(loader.IsDone());
}
