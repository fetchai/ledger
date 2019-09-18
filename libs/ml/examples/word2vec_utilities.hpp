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

#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <vector>

namespace fetch {
namespace ml {
namespace examples {

template <class TensorType>
std::vector<std::pair<typename TensorType::SizeType, typename TensorType::Type>> GetWordIDAnalogies(
    TensorType const &embeddings, typename TensorType::SizeType const &word1,
    typename TensorType::SizeType const &word2, typename TensorType::SizeType const &word3,
    typename TensorType::SizeType k) {
  using SizeType = typename TensorType::SizeType;
  using DataType = typename TensorType::Type;

  // get word vectors for word_ids
  TensorType word1_vec = embeddings.Slice(word1, 1).Copy();
  TensorType word2_vec = embeddings.Slice(word2, 1).Copy();
  TensorType word3_vec = embeddings.Slice(word3, 1).Copy();

  word1_vec /= fetch::math::L2Norm(word1_vec);
  word2_vec /= fetch::math::L2Norm(word2_vec);
  word3_vec /= fetch::math::L2Norm(word3_vec);

  TensorType word4_vec = word2_vec - word1_vec + word3_vec;
  std::vector<std::pair<SizeType, typename TensorType::Type>> output;
  if (k > 1)
  {
    output = fetch::math::clustering::KNNCosine(embeddings, word4_vec, k);
  }
  else
  {
    // compute distances
    SizeType feature_axis;
    SizeType data_axis;
    if (word4_vec.shape().at(0) == 1)
    {
      feature_axis = 1;
    }
    else
    {
      feature_axis = 0;
    }

    DataType min_dist = 100;
    SizeType min_ind = 0;
    data_axis = 1 - feature_axis;
    for (SizeType i = 0; i < embeddings.shape().at(data_axis); ++i)
    {
      DataType d = fetch::math::distance::Cosine(word4_vec, embeddings.Slice(i, data_axis).Copy());
      if (d < min_dist)
      {
        min_dist = d;
        min_ind = i;
      }
    }
    output.push_back(std::make_pair(min_ind, min_dist));
  }
  return output;
}

template <class TensorType>
void TestWithAnalogies(dataloaders::GraphW2VLoader<typename TensorType::Type> const &dl,
                       TensorType const &                                            embeddings,
                       std::string const &analogy_file = "./datasets/questions-words.txt")
{
  using SizeType = typename TensorType::SizeType;
  using DataType = typename TensorType::Type;

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
  SizeType    unknown_count = 0;
  SizeType    success_count = 0;
  SizeType    fail_count    = 0;

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
        unknown_count++;
        continue;
      }

      // get id for words
      SizeType word1_idx = dl.IndexFromWord(word1);
      SizeType word2_idx = dl.IndexFromWord(word2);
      SizeType word3_idx = dl.IndexFromWord(word3);
      SizeType word4_idx = dl.IndexFromWord(word4);

      std::vector<std::pair<SizeType, DataType>> result =
          GetWordIDAnalogies<TensorType>(embeddings, word1_idx, word2_idx, word3_idx, 1);

      if (result[0].first == word4_idx)
      {
        success_count++;
      }
      else
      {
        fail_count++;
      }
    }
  }
  std::cout << "Unknown, success, fail " << unknown_count << ' ' << success_count << ' ' << fail_count
            << std::endl;
}

//
//void SaveGraphToFile(GraphType &g, std::string const file_name)
//{
//  fetch::ml::GraphSaveableParams<TensorType> gsp1 = g.GetGraphSaveableParams();
//
//  fetch::serializers::LargeObjectSerializeHelper losh;
//
//  losh << gsp1;
//
//  std::ofstream outFile(file_name, std::ios::out | std::ios::binary);
//  outFile.write(losh.buffer.data().char_pointer(), std::streamsize(losh.buffer.size()));
//  outFile.close();
//  std::cout << "Buffer size " << losh.buffer.size() << std::endl;
//  std::cout << "Finished writing to file " << file_name << std::endl;
//}

}  // namespace examples
}  // namespace ml
}  // namespace fetch
