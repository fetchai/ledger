#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "math/metrics/l2_norm.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"
#include "ml/layers/skip_gram.hpp"

#include <chrono>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <vector>

namespace fetch {
namespace ml {
namespace utilities {

template <class TensorType>
TensorType const &GetEmbeddings(Graph<TensorType> const &g, std::string const &skip_gram_name)
{
  // first get hold of the skipgram layer by searching the return name in the graph
  std::shared_ptr<fetch::ml::layers::SkipGram<TensorType>> sg_layer =
      std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<TensorType>>(
          (g.GetNode(skip_gram_name))->GetOp());

  // next get hold of the embeddings
  std::shared_ptr<fetch::ml::ops::Embeddings<TensorType>> embeddings =
      sg_layer->GetEmbeddings(sg_layer);
  return embeddings->GetWeights();
}

template <class TensorType>
std::vector<std::pair<math::SizeType, typename TensorType::Type>> GetWordIDAnalogies(
    TensorType const &embeddings, math::SizeType const &word1, math::SizeType const &word2,
    math::SizeType const &word3, math::SizeType k)
{
  using SizeType = math::SizeType;
  using DataType = typename TensorType::Type;

  // get word vectors for word_ids
  TensorType word1_vec = embeddings.Slice(word1, 1).Copy();
  TensorType word2_vec = embeddings.Slice(word2, 1).Copy();
  TensorType word3_vec = embeddings.Slice(word3, 1).Copy();

  word1_vec /= math::L2Norm(word1_vec);
  word2_vec /= math::L2Norm(word2_vec);
  word3_vec /= math::L2Norm(word3_vec);

  TensorType                                 word4_vec = word2_vec - word1_vec + word3_vec;
  std::vector<std::pair<SizeType, DataType>> output =
      math::clustering::KNNCosine(embeddings, word4_vec, k);

  return output;
}

template <class TensorType>
std::string WordAnalogyTest(dataloaders::Vocab const &vcb, TensorType const &embeddings,
                            std::string const &word1, std::string const &word2,
                            std::string const &word3, math::SizeType k)
{
  using SizeType = math::SizeType;
  using DataType = typename TensorType::Type;

  std::stringstream outstream;

  outstream << "Find word that is to " << word3 << " what " << word2 << " is to " << word1
            << std::endl;

  if (!vcb.WordKnown(word1) || !vcb.WordKnown(word2) || !vcb.WordKnown(word3))
  {
    outstream << "Error: Not all to-be-tested words are in vocabulary." << std::endl;
  }
  else
  {
    // get id for words
    SizeType word1_idx = vcb.IndexFromWord(word1);
    SizeType word2_idx = vcb.IndexFromWord(word2);
    SizeType word3_idx = vcb.IndexFromWord(word3);

    std::vector<std::pair<SizeType, DataType>> output =
        GetWordIDAnalogies<TensorType>(embeddings, word1_idx, word2_idx, word3_idx, k);

    for (SizeType l = 0; l < output.size(); ++l)
    {
      outstream << "rank: " << l << ", "
                << "distance, " << output.at(l).second << ": "
                << vcb.WordFromIndex(output.at(l).first) << std::endl;
    }
  }
  return outstream.str();
}

template <class TensorType>
std::string KNNTest(dataloaders::Vocab const &vcb, TensorType const &embeddings,
                    std::string const &word0, math::SizeType k)
{
  using SizeType = math::SizeType;
  using DataType = typename TensorType::Type;

  std::stringstream outstream;

  outstream << "Find words that are closest to \"" << word0 << "\" by cosine distance" << std::endl;

  if (!vcb.WordKnown(word0))
  {
    outstream << "Error: could not find \"" + word0 + "\" in vocabulary" << std::endl;
  }
  else
  {
    SizeType                                   idx        = vcb.IndexFromWord(word0);
    TensorType                                 one_vector = embeddings.Slice(idx, 1).Copy();
    std::vector<std::pair<SizeType, DataType>> output =
        math::clustering::KNNCosine(embeddings, one_vector, k);

    for (std::size_t l = 0; l < output.size(); ++l)
    {
      outstream << "rank: " << l << ", "
                << "distance, " << output.at(l).second << ": "
                << vcb.WordFromIndex(output.at(l).first) << std::endl;
    }
  }
  return outstream.str();
}

template <class TensorType>
std::pair<std::string, float> AnalogiesFileTest(dataloaders::Vocab const &vcb,
                                                TensorType const &        embeddings,
                                                std::string const &       analogy_file,
                                                bool                      verbose = false)
{
  using SizeType = math::SizeType;
  using DataType = typename TensorType::Type;

  std::stringstream outstream;

  outstream << "Testing with analogies" << std::endl;

  // read analogy file
  std::ifstream fp(analogy_file);
  if (fp.fail())
  {
    throw ml::exceptions::InvalidFile("Cannot open file " + analogy_file);
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

      if (!vcb.WordKnown(word1) || !vcb.WordKnown(word2) || !vcb.WordKnown(word3) ||
          !vcb.WordKnown(word4))
      {
        unknown_count += 1;
        continue;
      }

      if (verbose)
      {
        outstream << word1 << " " << word2 << " " << word3 << " " << word4 << std::endl;
      }

      // get id for words
      SizeType word1_idx = vcb.IndexFromWord(word1);
      SizeType word2_idx = vcb.IndexFromWord(word2);
      SizeType word3_idx = vcb.IndexFromWord(word3);
      SizeType word4_idx = vcb.IndexFromWord(word4);

      std::vector<std::pair<SizeType, DataType>> results =
          GetWordIDAnalogies<TensorType>(embeddings, word1_idx, word2_idx, word3_idx, 4);

      for (auto result : results)
      {
        SizeType idx = result.first;
        if (idx != word1_idx && idx != word2_idx && idx != word3_idx)
        {
          if (verbose)
          {
            outstream << "Result: " << vcb.WordFromIndex(result.first) << std::endl;
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
  outstream << "Unknown, success, fail: " << unknown_count << ' ' << success_count << ' '
            << fail_count << std::endl;

  float success_fraction =
      static_cast<float>(success_count) / static_cast<float>(success_count + fail_count);

  outstream << "Success percentage: " << success_fraction * 100.0f << "%" << std::endl;

  return std::make_pair(outstream.str(), success_fraction);
}

template <class TensorType>
void TestEmbeddings(Graph<TensorType> const &g, std::string const &skip_gram_name,
                    dataloaders::Vocab const &vcb, std::string const &word0,
                    std::string const &word1, std::string const &word2, std::string const &word3,
                    math::SizeType K, std::string const &analogies_test_file, bool verbose = true,
                    std::string const &outfile = "")
{
  TensorType const &weights = utilities::GetEmbeddings(g, skip_gram_name);

  std::string knn_results = utilities::KNNTest(vcb, weights, word0, K);

  std::string word_analogy_results =
      utilities::WordAnalogyTest(vcb, weights, word1, word2, word3, K);

  auto analogies_file_results = utilities::AnalogiesFileTest(vcb, weights, analogies_test_file);

  if (verbose)
  {
    std::cout << std::endl << knn_results << std::endl;
    std::cout << std::endl << word_analogy_results << std::endl;
    std::cout << std::endl << analogies_file_results.first << std::endl;
  }
  if (!outfile.empty())
  {
    std::ofstream test_file(outfile, std::ofstream::out | std::ofstream::app);
    if (test_file)
    {
      test_file << std::endl << knn_results << std::endl;
      test_file << std::endl << word_analogy_results << std::endl;
      test_file << std::endl << analogies_file_results.first << std::endl;
    }
  }
}

std::string ReadFile(std::string const &path);

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
