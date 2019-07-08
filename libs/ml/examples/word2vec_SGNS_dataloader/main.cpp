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

#include "file_loader.hpp"
#include "model_saver.hpp"

#include "math/clustering/knn.hpp"
#include "math/matrix_operations.hpp"
#include "math/ml/loss_functions/l2_norm.hpp"
#include "math/tensor.hpp"
#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/skip_gram.hpp"
#include "ml/ops/loss_functions/cross_entropy.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"

#include <iostream>
#include <string>
#include <vector>

using namespace fetch::ml;
using namespace fetch::ml::dataloaders;
using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using DataType  = double;
using ArrayType = fetch::math::Tensor<DataType>;
using SizeType  = typename ArrayType::SizeType;

////////////////////////
/// MODEL DEFINITION ///
////////////////////////

std::string Model(fetch::ml::Graph<ArrayType> &g, SizeType embeddings_size, SizeType vocab_size)
{
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Context", {});
  std::string ret_name = g.AddNode<fetch::ml::layers::SkipGram<ArrayType>>(
      "SkipGram", {"Input", "Context"}, SizeType(1), SizeType(1), embeddings_size, vocab_size);

  return ret_name;
}

void PrintWordAnalogy(GraphW2VLoader<DataType> const &dl, ArrayType const &embeddings,
                      std::string const &word1, std::string const &word2, std::string const &word3,
                      SizeType k)
{
  ArrayType arr = embeddings;

  if (!dl.WordKnown(word1) || !dl.WordKnown(word2) || !dl.WordKnown(word3))
  {
    throw std::runtime_error("WARNING! not all to-be-tested words are in vocabulary");
  }
  else
  {
    std::cout << "Find word that to " << word3 << " is what " << word2 << " is to " << word1
              << std::endl;
  }

  // get id for words
  SizeType word1_idx = dl.IndexFromWord(word1);
  SizeType word2_idx = dl.IndexFromWord(word2);
  SizeType word3_idx = dl.IndexFromWord(word3);

  // get word vectors for words
  ArrayType word1_vec = embeddings.Slice(word1_idx, 1).Copy();
  ArrayType word2_vec = embeddings.Slice(word2_idx, 1).Copy();
  ArrayType word3_vec = embeddings.Slice(word3_idx, 1).Copy();

  word1_vec /= fetch::math::L2Norm(word1_vec);
  word2_vec /= fetch::math::L2Norm(word2_vec);
  word3_vec /= fetch::math::L2Norm(word3_vec);

  ArrayType word4_vec = word2_vec - word1_vec + word3_vec;

  std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> output =
      fetch::math::clustering::KNNCosine(arr, word4_vec, k);

  for (std::size_t l = 0; l < output.size(); ++l)
  {
    std::cout << "rank: " << l << ", "
              << "distance, " << output.at(l).second << ": " << dl.WordFromIndex(output.at(l).first)
              << std::endl;
  }
}

void PrintKNN(GraphW2VLoader<DataType> const &dl, ArrayType const &embeddings,
              std::string const &word0, SizeType k)
{
  ArrayType arr = embeddings;

  if (dl.IndexFromWord(word0) == fetch::math::numeric_max<SizeType>())
  {
    throw std::runtime_error("WARNING! could not find [" + word0 + "] in vocabulary");
  }

  SizeType  idx        = dl.IndexFromWord(word0);
  ArrayType one_vector = embeddings.Slice(idx, 1).Copy();
  std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> output =
      fetch::math::clustering::KNNCosine(arr, one_vector, k);

  for (std::size_t l = 0; l < output.size(); ++l)
  {
    std::cout << "rank: " << l << ", "
              << "distance, " << output.at(l).second << ": " << dl.WordFromIndex(output.at(l).first)
              << std::endl;
  }
}

void TestEmbeddings(Graph<ArrayType> const &g, std::string const &skip_gram_name,
                    GraphW2VLoader<DataType> const &dl, std::string word0, std::string word1,
                    std::string word2, std::string word3, SizeType K)
{

  // first get hold of the skipgram layer by searching the return name in the graph
  std::shared_ptr<fetch::ml::layers::SkipGram<ArrayType>> sg_layer =
      std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<ArrayType>>(g.GetNode(skip_gram_name));

  // next get hold of the embeddings
  std::shared_ptr<fetch::ml::ops::Embeddings<ArrayType>> embeddings =
      sg_layer->GetEmbeddings(sg_layer);

  std::cout << std::endl;
  PrintKNN(dl, embeddings->get_weights(), word0, K);
  std::cout << std::endl;
  PrintWordAnalogy(dl, embeddings->get_weights(), word1, word2, word3, K);
}

std::string ReadFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

////////////////////////////////
/// PARAMETERS AND CONSTANTS ///
////////////////////////////////

struct TrainingParams
{
  SizeType max_word_count       = 100000000;  // maximum number to be trained
  SizeType negative_sample_size = 5;          // number of negative sample per word-context pair
  SizeType window_size          = 5;          // window size for context sampling
  bool     train_mode           = true;       // reserve for future compatibility with CBOW
  DataType freq_thresh          = 1e-3;       // frequency threshold for subsampling
  SizeType min_count            = 5;          // infrequent word removal threshold

  SizeType batch_size      = 100000;  // training data batch size
  SizeType embedding_size  = 100;     // dimension of embedding vec
  SizeType training_epochs = 1;
  SizeType test_frequency  = 1;
  DataType starting_learning_rate_per_sample =
      0.025;  // these are the learning rates we have for each sample
  DataType ending_learning_rate_per_sample = 0.0001;
  DataType starting_learning_rate;  // this is the true learning rate set for the graph training
  DataType ending_learning_rate;

  fetch::ml::optimisers::LearningRateParam<DataType> learning_rate_param{
      fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::LINEAR};

  SizeType    k        = 20;       // how many nearest neighbours to compare against
  std::string word0    = "three";  // test word to consider
  std::string word1    = "king";
  std::string word2    = "queen";
  std::string word3    = "father";
  std::string save_loc = "./model.fba";  // save file location for exporting graph
};

int main(int argc, char **argv)
{

  std::string train_file;
  if (argc == 2)
  {
    train_file = argv[1];
  }
  else
  {
    throw std::runtime_error("must specify filename as training text");
  }

  std::cout << "FETCH Word2Vec Demo" << std::endl;

  TrainingParams tp;

  ///////////////////////////////////////
  /// CONVERT TEXT INTO TRAINING DATA ///
  ///////////////////////////////////////

  std::cout << "Setting up training data...: " << std::endl;

  GraphW2VLoader<DataType> data_loader(tp.window_size, tp.negative_sample_size, tp.freq_thresh,
                                       tp.max_word_count, tp.train_mode);
  // set up dataloader
  /// DATA LOADING ///
  data_loader.BuildVocab({ReadFile(train_file)}, tp.min_count);


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
  tp.learning_rate_param.linear_decay_rate =
      static_cast<DataType>(1) /
      data_loader
          .EstimatedSampleNumber();  // this decay rate gurantee the lr is reduced to zero by the
                                     // end of an epoch (despit capping by ending learning rate)
  std::cout << "data_loader.EstimatedSampleNumber(): " << data_loader.EstimatedSampleNumber()
            << std::endl;

  ////////////////////////////////
  /// SETUP MODEL ARCHITECTURE ///
  ////////////////////////////////

  // set up model architecture
  std::cout << "building model architecture...: " << std::endl;
  auto        g          = std::make_shared<fetch::ml::Graph<ArrayType>>();
  std::string model_name = Model(*g, tp.embedding_size, data_loader.vocab_size());

  // set up loss
  CrossEntropy<ArrayType> criterion;

  /////////////////////////////////
  /// TRAIN THE WORD EMBEDDINGS ///
  /////////////////////////////////

  std::cout << "beginning training...: " << std::endl;

  // Initialise Optimiser
  fetch::ml::optimisers::SGDOptimiser<ArrayType, fetch::ml::ops::CrossEntropy<ArrayType>> optimiser(
      g, {"Input", "Context"}, model_name, tp.learning_rate_param);

  // Training loop
  for (SizeType i{0}; i < tp.training_epochs; i++)
  {
    std::cout << "start training for epoch no.: " << i << std::endl;
    optimiser.Run(data_loader, tp.batch_size);
    std::cout << std::endl;
    // Test trained embeddings
    if (i % tp.test_frequency == 0)
    {
      TestEmbeddings(*g, model_name, data_loader, tp.word0, tp.word1, tp.word2, tp.word3, tp.k);
      TestEmbeddings(*g, model_name, data_loader, "france", "france", "paris", "italy", tp.k);
    }
  }

  //////////////////////////////////////
  /// EXTRACT THE TRAINED EMBEDDINGS ///
  //////////////////////////////////////

  return 0;
}
