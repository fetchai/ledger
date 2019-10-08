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

#include "math/matrix_operations.hpp"
#include "ml/exceptions/exceptions.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

template <typename TensorType>
void NormVector(TensorType &vector)
{
  typename TensorType::Type l2 = 0;
  for (auto &val : vector)
  {
    l2 += (val * val);
  }
  l2 = static_cast<typename TensorType::Type>(sqrt(l2));
  for (auto &val : vector)
  {
    val /= l2;
  }
}

template <typename TensorType, typename DataLoaderType, typename SizeType>
void EvalAnalogy(DataLoaderType const &data_loader, TensorType const &embeds, SizeType const top_k,
                 std::vector<std::string> const &test_words)
{
  assert(test_words.size() == 3);

  auto embeddings = embeds.Copy();

  std::string word1     = test_words[0];
  std::string word2     = test_words[1];
  std::string word3     = test_words[2];
  SizeType    word1_idx = data_loader.IndexFromWord(word1);
  if (word1_idx == 0)
  {
    throw fetch::ml::exceptions::InvalidInput("word1 not found");
  }
  SizeType word2_idx = data_loader.IndexFromWord(word2);
  if (word2_idx == 0)
  {
    throw fetch::ml::exceptions::InvalidInput("word2 not found");
  }
  SizeType word3_idx = data_loader.IndexFromWord(word3);
  if (word3_idx == 0)
  {
    throw fetch::ml::exceptions::InvalidInput("word3 not found");
  }

  auto word_vector_1 = embeddings.View(word1_idx).Copy();
  auto word_vector_2 = embeddings.View(word2_idx).Copy();
  auto word_vector_3 = embeddings.View(word3_idx).Copy();

  // normalise the test target vectors
  NormVector(word_vector_1);
  NormVector(word_vector_2);
  NormVector(word_vector_3);

  /// Vector Math analogy: Paris - France + Italy should give us rome ///
  auto analogy_target_vector = word_vector_2 - word_vector_1 + word_vector_3;

  NormVector(analogy_target_vector);
  auto output = fetch::math::clustering::KNNCosine(embeddings, analogy_target_vector, top_k);

  std::cout << "KNN results: " << std::endl;
  for (std::size_t l = 0; l < output.size(); ++l)
  {
    std::cout << "rank: " << l << ", "
              << "distance, " << output.at(l).second << ": "
              << data_loader.WordFromIndex(output.at(l).first) << std::endl;
  }
}
