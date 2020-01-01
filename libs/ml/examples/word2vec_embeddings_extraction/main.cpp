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

#include "core/commandline/parameter_parser.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "ml/utilities/word2vec_utilities.hpp"

#include <string>

using namespace fetch::ml;
using namespace fetch::ml::dataloaders;

using DataType   = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType   = fetch::math::SizeType;

TensorType word_embedding(std::string const &word, TensorType const &embeddings, Vocab const &vcb)
{
  SizeType word_idx = vcb.IndexFromWord(word);
  return embeddings.Slice(word_idx, 1).Copy();
}

int main(int argc, char **argv)
{
  fetch::commandline::ParamsParser parser;
  parser.Parse(argc, argv);

  std::string vocab_file      = parser.GetParam("vocab", "");
  std::string embeddings_file = parser.GetParam("embeddings", "");

  Vocab      vcb;
  TensorType embeddings;

  if (!vocab_file.empty())
  {
    std::cout << "Loading vocab... " << std::endl;
    vcb.Load(vocab_file);
  }
  else
  {
    throw exceptions::InvalidFile("Please provide a vocab file with -vocab");
  }

  if (!embeddings_file.empty())
  {
    std::cout << "Loading embeddings..." << std::endl;
    embeddings = TensorType::FromString(utilities::ReadFile(embeddings_file));
  }
  else
  {
    throw exceptions::InvalidFile("Please provide an embeddings file with -embeddings");
  }

  if (vcb.GetVocabCount() != embeddings.shape()[1])
  {
    throw exceptions::InvalidInput(
        "Vocab size does not match embeddings size: " + std::to_string(vcb.GetVocabCount()) + " " +
        std::to_string(embeddings.shape()[1]));
  }

  std::string test_word = "king";
  if (!vcb.WordKnown(test_word))
  {
    std::cout << "test_word not in vocab: " << test_word << std::endl;
  }
  else
  {

    TensorType one_embedding = word_embedding(test_word, embeddings, vcb);
    std::cout << "Embedding for " << test_word << " is: " << one_embedding.ToString() << std::endl;
  }

  std::string knn_results = utilities::KNNTest(vcb, embeddings, "three", 20);
  std::cout << std::endl << knn_results << std::endl;

  std::string word_analogy_results =
      utilities::WordAnalogyTest(vcb, embeddings, "king", "queen", "father", 20);
  std::cout << std::endl << word_analogy_results << std::endl;
}
