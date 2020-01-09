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

#include "math/matrix_operations.hpp"
#include "math/tensor/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"
#include "ml/layers/skip_gram.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/optimisation/lazy_adam_optimiser.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"
#include "ml/utilities/graph_saver.hpp"
#include "ml/utilities/word2vec_utilities.hpp"

#include <iostream>
#include <string>
#include <utility>

using namespace fetch::ml;
using namespace fetch::ml::dataloaders;
using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType   = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType   = fetch::math::SizeType;

////////////////////////
/// MODEL DEFINITION ///
////////////////////////

std::pair<std::string, std::string> Model(fetch::ml::Graph<TensorType> &g, SizeType embeddings_size,
                                          SizeType vocab_size)
{
  g.AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  g.AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Context", {});
  std::string label    = g.AddNode<PlaceHolder<TensorType>>("Label", {});
  std::string skipgram = g.AddNode<fetch::ml::layers::SkipGram<TensorType>>(
      "SkipGram", {"Input", "Context"}, SizeType(1), SizeType(1), embeddings_size, vocab_size);

  std::string error = g.AddNode<CrossEntropyLoss<TensorType>>("Error", {skipgram, label});

  return std::pair<std::string, std::string>(error, skipgram);
}

////////////////////////////////
/// PARAMETERS AND CONSTANTS ///
////////////////////////////////

struct TrainingParams
{
  // window_size(2), embedding_size(500) and min_count(100) come from the Levy et. al. paper here
  // (https://www.aclweb.org/anthology/Q15-1016) which has state-of-the-art scores for word
  // embedding and uses the wikipedia dataset (documents_utf8_filtered_20pageviews.csv)
  SizeType max_word_count = fetch::math::numeric_max<SizeType>();  // maximum number to be trained
  SizeType negative_sample_size = 5;  // number of negative sample per word-context pair
  SizeType window_size          = 2;  // window size for context sampling
  DataType freq_thresh =
      fetch::math::Type<DataType>("0.001");  // frequency threshold for subsampling
  SizeType min_count = 100;                  // infrequent word removal threshold

  SizeType batch_size            = 10000;  // training data batch size
  SizeType embedding_size        = 500;    // dimension of embedding vec
  SizeType training_epochs       = 1;
  SizeType test_frequency        = 1;
  SizeType graph_saves_per_epoch = 10;

  // these are the learning rates we have for each sample
  DataType starting_learning_rate_per_sample = fetch::math::Type<DataType>("0.0025");
  DataType ending_learning_rate_per_sample   = fetch::math::Type<DataType>("0.0001");
  // this is the true learning rate set for the graph training
  DataType starting_learning_rate;
  DataType ending_learning_rate;

  fetch::ml::optimisers::LearningRateParam<DataType> learning_rate_param{
      fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::LINEAR};

  SizeType    k     = 20;       // how many nearest neighbours to compare against
  std::string word0 = "three";  // test word to consider
  std::string word1 = "king";
  std::string word2 = "queen";
  std::string word3 = "father";
};

int main(int argc, char **argv)
{

  std::string train_file;
  std::string save_file;
  std::string analogies_test_file;
  if (argc == 4)
  {
    train_file          = argv[1];
    save_file           = argv[2];
    analogies_test_file = argv[3];
  }
  else
  {
    throw fetch::ml::exceptions::InvalidInput(
        "Args: data_file graph_save_file analogies_test_file");
  }

  std::cout << "FETCH Word2Vec Demo" << std::endl;

  TrainingParams tp;

  ///////////////////////////////////////
  /// CONVERT TEXT INTO TRAINING DATA ///
  ///////////////////////////////////////

  std::cout << "Setting up training data...: " << std::endl;

  GraphW2VLoader<TensorType> data_loader(tp.window_size, tp.negative_sample_size, tp.freq_thresh,
                                         tp.max_word_count);
  // set up dataloader
  /// DATA LOADING ///
  data_loader.BuildVocabAndData({utilities::ReadFile(train_file)}, tp.min_count);
  std::string vocab_file = "/tmp/vocab.txt";
  std::cout << "Saving vocab to vocab_file: " << vocab_file << std::endl;
  data_loader.SaveVocab(vocab_file);

  /////////////////////////////////////////
  /// SET UP PROPER TRAINING PARAMETERS ///
  /////////////////////////////////////////

  // calc the true starting learning rate
  tp.starting_learning_rate =
      static_cast<DataType>(tp.batch_size) * tp.starting_learning_rate_per_sample;
  tp.ending_learning_rate =
      static_cast<DataType>(tp.batch_size) * tp.ending_learning_rate_per_sample;
  tp.learning_rate_param.starting_learning_rate = tp.starting_learning_rate;
  tp.learning_rate_param.ending_learning_rate   = tp.ending_learning_rate;

  // calc the compatiable linear lr decay
  DataType est_total_samples               = data_loader.EstimatedSampleNumber();
  tp.learning_rate_param.linear_decay_rate = static_cast<DataType>(1) / est_total_samples;
  // this decay rate gurantees that the lr is reduced to zero by the
  // end of an epoch (despite capping by ending learning rate)

  ////////////////////////////////
  /// SETUP MODEL ARCHITECTURE ///
  ////////////////////////////////

  // set up model architecture
  std::cout << "building model architecture...: " << std::endl;
  auto                                g = std::make_shared<fetch::ml::Graph<TensorType>>();
  std::pair<std::string, std::string> error_and_skipgram_layer =
      Model(*g, tp.embedding_size, data_loader.vocab_size());
  std::string error          = error_and_skipgram_layer.first;
  std::string skipgram_layer = error_and_skipgram_layer.second;

  /////////////////////////////////
  /// TRAIN THE WORD EMBEDDINGS ///
  /////////////////////////////////

  std::cout << "beginning training...: " << std::endl;

  // Initialise Optimiser
  fetch::ml::optimisers::LazyAdamOptimiser<TensorType> optimiser(g, {"Input", "Context"}, "Label",
                                                                 error, tp.learning_rate_param);

  SizeType n_batches              = static_cast<SizeType>(est_total_samples) / tp.batch_size;
  SizeType samples_per_graph_save = n_batches / tp.graph_saves_per_epoch * tp.batch_size;

  // Training loop
  for (SizeType i{0}; i < tp.training_epochs; i++)
  {
    std::cout << "Start training for epoch no.: " << i << std::endl;

    for (SizeType j{0}; j < tp.graph_saves_per_epoch - 1; j++)
    {
      optimiser.Run(data_loader, tp.batch_size, samples_per_graph_save);
      fetch::ml::utilities::SaveGraph(*g, save_file + std::to_string(i) + "_" + std::to_string(j));
    }

    // final run with remainder of samples
    optimiser.Run(data_loader, tp.batch_size);

    // Test trained embeddings
    if (i % tp.test_frequency == 0)
    {
      fetch::ml::utilities::TestEmbeddings(*g, skipgram_layer, *(data_loader.GetVocab()), tp.word0,
                                           tp.word1, tp.word2, tp.word3, tp.k, analogies_test_file);
    }

    fetch::ml::utilities::SaveGraph(*g, save_file + std::to_string(i));
  }

  return 0;
}
