#pragma once
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

#include <iostream>
#include <math.h>
#include <string>
#include <vector>

template <typename ArrayType, typename DataLoaderType, typename SizeType>
void EvalAnalogy(DataLoaderType &data_loader, ArrayType &embeds, SizeType top_k,
                 std::vector<std::string> &test_words)
{

  assert(test_words.size() == 3);

  using DataType = typename ArrayType::Type;
  /// first we L2 normalise all embeddings ///
  SizeType vocab_size     = embeds.shape()[1];
  SizeType embedding_size = embeds.shape()[0];
  auto     embeddings     = embeds.Copy();
  for (std::size_t i = 0; i < vocab_size; ++i)
  {
    DataType l2 = 0;
    l2          = std::sqrt(l2);
    for (std::size_t k = 0; k < embedding_size; ++k)
    {
      l2 += (embeddings(k, i) * embeddings(k, i));
    }
    l2 = sqrt(l2);

    // divide each element by ss
    for (std::size_t k = 0; k < embedding_size; ++k)
    {
      embeddings(k, i) /= l2;
    }
  }

  std::string word1     = test_words[0];
  std::string word2     = test_words[1];
  std::string word3     = test_words[2];
  SizeType    word1_idx = data_loader.IndexFromWord(word1);
  if (word1_idx == 0)
  {
    throw std::runtime_error("word1 not found");
  }
  SizeType word2_idx = data_loader.IndexFromWord(word2);
  if (word2_idx == 0)
  {
    throw std::runtime_error("word2 not found");
  }
  SizeType word3_idx = data_loader.IndexFromWord(word3);
  if (word3_idx == 0)
  {
    throw std::runtime_error("word3 not found");
  }

  auto word_vector_1 = embeddings.Slice(word1_idx, 1).Copy();
  auto word_vector_2 = embeddings.Slice(word2_idx, 1).Copy();
  auto word_vector_3 = embeddings.Slice(word3_idx, 1).Copy();

  std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> output;

  /// Vector Math analogy: Paris - France + Italy should give us rome ///
  auto analogy_target_vector = word_vector_2 - word_vector_1 + word_vector_3;

  // normalise the analogy target vector
  DataType l2 = 0;
  l2          = std::sqrt(l2);
  for (auto &val : analogy_target_vector)
  {
    l2 += (val * val);
  }
  l2 = sqrt(l2);
  for (auto &val : analogy_target_vector)
  {
    val /= l2;
  }

  // TODO: we should be able to calculate this all in one call to KNNCosine - but result is
  // different
  //  output = fetch::math::clustering::KNNCosine(embeddings.Transpose(),
  //  analogy_target_vector.Transpose(), k);

  /// manual calculate similarity ///

  //
  DataType                                   cur_similarity;
  std::vector<std::pair<SizeType, DataType>> best_word_distances{};
  for (std::size_t m = 0; m < top_k; ++m)
  {
    best_word_distances.emplace_back(std::make_pair(0, fetch::math::numeric_lowest<DataType>()));
  }

  for (std::size_t i = 0; i < vocab_size; ++i)
  {
    cur_similarity = 0;
    if ((i == 0) || (i == word1_idx) || (i == word2_idx) || (i == word3_idx))
    {
      continue;
    }

    SizeType e_idx = 0;
    for (auto &val : analogy_target_vector)
    {
      cur_similarity += (val * embeddings(e_idx, i));
      e_idx++;
    }

    bool replaced = false;
    for (std::size_t j = 0; j < best_word_distances.size(); ++j)
    {
      if (cur_similarity > best_word_distances[j].second)
      {
        auto prev_best = best_word_distances[j];
        if (replaced)
        {
          best_word_distances[j] = prev_best;
        }
        else
        {
          best_word_distances[j] = std::make_pair(i, cur_similarity);
          replaced               = true;
        }
      }
    }
  }

  std::cout << word2 << " - " << word1 << " + " << word3 << " = : " << std::endl;
  for (std::size_t l = 0; l < best_word_distances.size(); ++l)
  {
    std::cout << "rank: " << l << ", "
              << "distance, " << best_word_distances.at(l).second << ": "
              << data_loader.WordFromIndex(best_word_distances.at(l).first) << std::endl;
  }
}