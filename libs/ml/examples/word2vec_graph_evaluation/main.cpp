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
#include "ml/layers/skip_gram.hpp"
#include "ml/utilities/word2vec_utilities.hpp"
#include "model_saver.hpp"

#include <stdexcept>
#include <string>

using namespace fetch::ml;
using namespace fetch::ml::examples;
using namespace fetch::ml::utilities;
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
    throw std::runtime_error("Args: graph_save_file data_file analogy_file");
  }

  std::shared_ptr<Graph<TensorType>> g_ptr = LoadModel<Graph<TensorType>>(graph_file);

  std::cout << "Setting up training data...: " << std::endl;

  GraphW2VLoader<DataType> data_loader(window_size, negative_sample_size, freq_thresh,
                                       max_word_count);

  // set up dataloader
  /// DATA LOADING ///
  data_loader.BuildVocabAndData({ReadFile(dataloader_file)}, min_count, false);
  std::string skip_gram_name = "SkipGram";

  // first get hold of the skipgram layer by searching the return name in the graph
  auto sg_layer = std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<TensorType>>(
      g_ptr->GetNode(skip_gram_name)->GetOp());

  // next get hold of the embeddings
  std::shared_ptr<fetch::ml::ops::Embeddings<TensorType>> embeddings =
      sg_layer->GetEmbeddings(sg_layer);

  DataType score = fetch::ml::utilities::TestWithAnalogies<TensorType>(
      data_loader, embeddings->GetWeights(), analogy_file);
  std::cout << "Score on analogies task: " << score * 100 << "%" << std::endl;
  PrintKNN(data_loader, embeddings->GetWeights(), "three", 20);
  fetch::ml::utilities::PrintWordAnalogy(data_loader, embeddings->GetWeights(), "king", "queen",
                                         "father", 20);
}
