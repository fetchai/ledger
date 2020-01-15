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


#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"

namespace fetch {
namespace ml {
namespace utilities {

template <class TensorType>
TensorType const &GetEmbeddings(Graph<TensorType> const &g, std::string const &skip_gram_name);

template <class TensorType>
std::vector<std::pair<math::SizeType, typename TensorType::Type>> GetWordIDAnalogies(
    TensorType const &embeddings, math::SizeType const &word1, math::SizeType const &word2,
    math::SizeType const &word3, math::SizeType k);

template <class TensorType>
std::string WordAnalogyTest(dataloaders::Vocab const &vcb, TensorType const &embeddings,
                            std::string const &word1, std::string const &word2,
                            std::string const &word3, math::SizeType k);

template <class TensorType>
std::string KNNTest(dataloaders::Vocab const &vcb, TensorType const &embeddings,
                    std::string const &word0, math::SizeType k);

template <class TensorType>
std::pair<std::string, float> AnalogiesFileTest(dataloaders::Vocab const &vcb,
                                                TensorType const &        embeddings,
                                                std::string const &       analogy_file,
                                                bool                      verbose = false);

template <class TensorType>
void TestEmbeddings(Graph<TensorType> const &g, std::string const &skip_gram_name,
                    dataloaders::Vocab const &vcb, std::string const &word0,
                    std::string const &word1, std::string const &word2, std::string const &word3,
                    math::SizeType K, std::string const &analogies_test_file, bool verbose = true,
                    std::string const &outfile = "");

std::string ReadFile(std::string const &path);

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
