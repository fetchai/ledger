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
#include "ml/core/graph.hpp"
#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "ml/utilities/graph_saver.hpp"
#include "ml/utilities/word2vec_utilities.hpp"

#include <string>

using namespace fetch::ml;
using namespace fetch::ml::dataloaders;

// Note: DataType needs to be the same as that used for Graph if -graph option is specified
using DataType   = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType   = fetch::math::SizeType;

int main(int argc, char **argv)
{
  fetch::commandline::ParamsParser parser;
  parser.Parse(argc, argv);

  std::string data_file       = parser.GetParam("data", "");
  std::string vocab_file      = parser.GetParam("vocab", "");
  std::string graph_file      = parser.GetParam("graph", "");
  std::string analogy_file    = parser.GetParam("analogies", "");
  std::string embeddings_file = parser.GetParam("embeddings", "");

  Vocab      vcb;
  TensorType embeddings;

  if (!vocab_file.empty())
  {
    std::cout << "Loading vocab... " << std::endl;
    vcb.Load(vocab_file);
  }
  else if (!data_file.empty())
  {
    std::cout << "Loading training data...: " << std::endl;

    // Note: these parameters need to be the same as the ones that the graph was trained with.
    SizeType max_word_count = fetch::math::numeric_max<SizeType>();  // maximum number to be trained
    SizeType window_size    = 2;    // window size for context sampling
    SizeType min_count      = 100;  // infrequent word removal threshold

    // These don't!
    SizeType negative_sample_size = 5;  // number of negative sample per word-context pair
    DataType freq_thresh =
        fetch::math::Type<DataType>("0.001");  // frequency threshold for subsampling

    GraphW2VLoader<TensorType> data_loader(window_size, negative_sample_size, freq_thresh,
                                           max_word_count);
    data_loader.BuildVocabAndData({utilities::ReadFile(data_file)}, min_count, false);
    vcb        = *(data_loader.GetVocab());
    vocab_file = "/tmp/vocab.txt";
    vcb.Save(vocab_file);
    std::cout << "Saved vocab to vocab_file: " << vocab_file << std::endl;
  }
  else
  {
    throw exceptions::InvalidFile(
        "Please provide a data file or a vocab file with -data or -vocab");
  }

  if (!embeddings_file.empty())
  {
    std::cout << "Loading embeddings..." << std::endl;
    embeddings = TensorType::FromString(utilities::ReadFile(embeddings_file));
  }
  else if (!graph_file.empty())
  {
    std::cout << "Loading graph..." << std::endl;
    std::shared_ptr<Graph<TensorType>> g_ptr =
        fetch::ml::utilities::LoadGraph<Graph<TensorType>>(graph_file);

    std::string skip_gram_name = "SkipGram";
    embeddings                 = utilities::GetEmbeddings(*g_ptr, skip_gram_name);
    embeddings_file            = "/tmp/embeddings.txt";
    std::ofstream t(embeddings_file);
    if (t.fail())
    {
      throw exceptions::InvalidFile("Cannot open file " + embeddings_file);
    }
    t << embeddings.ToString();
    t.close();
    std::cout << "Saved embeddings to embeddings_file: " << embeddings_file << std::endl;
  }
  else
  {
    throw exceptions::InvalidFile(
        "Please provide a graph file with -graph or embeddings file with -embeddings");
  }

  if (vcb.GetVocabCount() != embeddings.shape()[1])
  {
    throw exceptions::InvalidInput(
        "Vocab size does not match embeddings size: " + std::to_string(vcb.GetVocabCount()) + " " +
        std::to_string(embeddings.shape()[1]));
  }

  std::string knn_results = utilities::KNNTest(vcb, embeddings, "three", 20);
  std::cout << std::endl << knn_results << std::endl;

  std::string word_analogy_results =
      utilities::WordAnalogyTest(vcb, embeddings, "king", "queen", "father", 20);
  std::cout << std::endl << word_analogy_results << std::endl;

  if (!analogy_file.empty())
  {
    auto analogies_file_results = utilities::AnalogiesFileTest(vcb, embeddings, analogy_file);
    std::cout << std::endl << analogies_file_results.first << std::endl;
  }
  else
  {
    std::cout << "Skipping analogy tests as analogy file not provided" << std::endl;
  }
}
