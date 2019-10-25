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

#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "ml/layers/skip_gram.hpp"
#include "ml/utilities/graph_saver.hpp"
#include "ml/utilities/word2vec_utilities.hpp"

#include <stdexcept>
#include <string>

using namespace fetch::ml;
using namespace fetch::ml::dataloaders;

using DataType   = float;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType   = TensorType ::SizeType;

int main(int argc, char **argv)
{
  // Note: these paramters need to be the same as the ones that the graph was trained with.
  SizeType max_word_count = fetch::math::numeric_max<SizeType>();  // maximum number to be trained
  SizeType negative_sample_size = 5;      // number of negative sample per word-context pair
  SizeType window_size          = 2;      // window size for context sampling
  DataType freq_thresh          = 1e-3f;  // frequency threshold for subsampling
  SizeType min_count            = 100;    // infrequent word removal threshold

  std::string graph_file;
  std::string dataloader_file;
  std::string analogy_file;

  if (argc == 4)
  {
    graph_file      = argv[1];
    dataloader_file = argv[2];
    analogy_file    = argv[3];
  }
  else
  {
    throw exceptions::InvalidFile("Args: graph_save_file data_file analogy_file");
  }

  std::shared_ptr<Graph<TensorType>> g_ptr =
      fetch::ml::utilities::LoadGraph<Graph<TensorType>>(graph_file);

  std::cout << "Setting up training data...: " << std::endl;

  GraphW2VLoader<TensorType> data_loader(window_size, negative_sample_size, freq_thresh,
                                         max_word_count);

  // set up dataloader
  /// DATA LOADING ///
  data_loader.BuildVocabAndData({utilities::ReadFile(dataloader_file)}, min_count, false);
  std::string skip_gram_name = "SkipGram";

  TensorType const &weights = utilities::GetEmbeddings(*g_ptr, skip_gram_name);

  std::string knn_results = utilities::KNNTest(data_loader, weights, "three", 20);
  std::cout << std::endl << knn_results << std::endl;

  std::string word_analogy_results =
      utilities::WordAnalogyTest(data_loader, weights, "king", "queen", "father", 20);
  std::cout << std::endl << word_analogy_results << std::endl;

  auto analogies_file_results = utilities::AnalogiesFileTest(data_loader, weights, analogy_file);
  std::cout << std::endl << analogies_file_results.first << std::endl;
}
