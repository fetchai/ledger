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

#include "math/clustering/knn.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"

#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <vector>

namespace fetch {
namespace ml {
namespace utilities {

template <class TensorType>
std::vector<std::pair<typename TensorType::SizeType, typename TensorType::Type>> GetWordIDAnalogies(
    TensorType const &embeddings, typename TensorType::SizeType const &word1,
    typename TensorType::SizeType const &word2, typename TensorType::SizeType const &word3,
    typename TensorType::SizeType k)
{
  using SizeType = typename TensorType::SizeType;
  using DataType = typename TensorType::Type;

  // get word vectors for word_ids
  TensorType word1_vec = embeddings.Slice(word1, 1).Copy();
  TensorType word2_vec = embeddings.Slice(word2, 1).Copy();
  TensorType word3_vec = embeddings.Slice(word3, 1).Copy();

  word1_vec /= fetch::math::L2Norm(word1_vec);
  word2_vec /= fetch::math::L2Norm(word2_vec);
  word3_vec /= fetch::math::L2Norm(word3_vec);

  TensorType                                 word4_vec = word2_vec - word1_vec + word3_vec;
  std::vector<std::pair<SizeType, DataType>> output =
      fetch::math::clustering::KNNCosine(embeddings, word4_vec, k);

  return output;
}

template <class TensorType>
void PrintWordAnalogy(dataloaders::GraphW2VLoader<typename TensorType::Type> const &dl,
                      TensorType const &embeddings, std::string const &word1,
                      std::string const &word2, std::string const &word3,
                      typename TensorType::SizeType k)
{
  using SizeType = typename TensorType::SizeType;
  using DataType = typename TensorType::Type;

  std::cout << "Find word that is to " << word3 << " what " << word2 << " is to " << word1
            << std::endl;

  if (!dl.WordKnown(word1) || !dl.WordKnown(word2) || !dl.WordKnown(word3))
  {
    std::cout << "Error: Not all to-be-tested words are in vocabulary." << std::endl;
  }
  else
  {
    // get id for words
    SizeType word1_idx = dl.IndexFromWord(word1);
    SizeType word2_idx = dl.IndexFromWord(word2);
    SizeType word3_idx = dl.IndexFromWord(word3);

    std::vector<std::pair<SizeType, DataType>> output =
        GetWordIDAnalogies<TensorType>(embeddings, word1_idx, word2_idx, word3_idx, k);

    for (SizeType l = 0; l < output.size(); ++l)
    {
      std::cout << "rank: " << l << ", "
                << "distance, " << output.at(l).second << ": "
                << dl.WordFromIndex(output.at(l).first) << std::endl;
    }
  }
}

template <class TensorType>
void PrintKNN(dataloaders::GraphW2VLoader<typename TensorType::Type> const &dl,
              TensorType const &embeddings, std::string const &word0,
              typename TensorType::SizeType k)
{
  using SizeType = typename TensorType::SizeType;
  using DataType = typename TensorType::Type;

  std::cout << "Find words that are closest to \"" << word0 << "\" by cosine distance" << std::endl;

  if (!dl.WordKnown(word0))
  {
    std::cout << "Error: could not find \"" + word0 + "\" in vocabulary" << std::endl;
  }
  else
  {
    SizeType                                   idx        = dl.IndexFromWord(word0);
    TensorType                                 one_vector = embeddings.Slice(idx, 1).Copy();
    std::vector<std::pair<SizeType, DataType>> output =
        fetch::math::clustering::KNNCosine(embeddings, one_vector, k);

    for (std::size_t l = 0; l < output.size(); ++l)
    {
      std::cout << "rank: " << l << ", "
                << "distance, " << output.at(l).second << ": "
                << dl.WordFromIndex(output.at(l).first) << std::endl;
    }
  }
}

template <class TensorType>
typename TensorType::Type TestWithAnalogies(
    dataloaders::GraphW2VLoader<typename TensorType::Type> const &dl, TensorType const &embeddings,
    std::string const &analogy_file, bool verbose = false)
{
  using SizeType = typename TensorType::SizeType;
  using DataType = typename TensorType::Type;

  std::cout << "Testing with analogies" << std::endl;

  // read analogy file
  std::ifstream fp(analogy_file);
  if (fp.fail())
  {
    throw std::runtime_error("Cannot open file " + analogy_file);
  }

  std::string word1;
  std::string word2;
  std::string word3;
  std::string word4;
  SizeType    unknown_count{0};
  SizeType    success_count{0};
  SizeType    fail_count{0};

  std::string buf;
  while (std::getline(fp, buf, '\n'))
  {
    if (!buf.empty() && buf[0] != ':')
    {
      std::stringstream ss(buf);

      ss >> word1 >> word2 >> word3 >> word4;

      if (!dl.WordKnown(word1) || !dl.WordKnown(word2) || !dl.WordKnown(word3) ||
          !dl.WordKnown(word4))
      {
        unknown_count += 1;
        continue;
      }

      if (verbose)
      {
        std::cout << word1 << " " << word2 << " " << word3 << " " << word4 << std::endl;
      }

      // get id for words
      SizeType word1_idx = dl.IndexFromWord(word1);
      SizeType word2_idx = dl.IndexFromWord(word2);
      SizeType word3_idx = dl.IndexFromWord(word3);
      SizeType word4_idx = dl.IndexFromWord(word4);

      std::vector<std::pair<SizeType, DataType>> results =
          GetWordIDAnalogies<TensorType>(embeddings, word1_idx, word2_idx, word3_idx, 4);

      for (auto result : results)
      {
        SizeType idx = result.first;
        if (idx != word1_idx && idx != word2_idx && idx != word3_idx)
        {
          if (verbose)
          {
            std::cout << "Result: " << dl.WordFromIndex(result.first) << std::endl;
          }
          if (idx == word4_idx)
          {
            success_count += 1;
          }
          else
          {
            fail_count += 1;
          }
          break;
        }
      }
    }
  }
  std::cout << "Unknown, success, fail: " << unknown_count << ' ' << success_count << ' '
            << fail_count << std::endl;

  return static_cast<DataType>(success_count) / static_cast<DataType>(success_count + fail_count);
}

inline std::string ReadFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
