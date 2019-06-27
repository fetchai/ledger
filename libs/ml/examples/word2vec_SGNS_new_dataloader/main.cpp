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
#include "math/tensor.hpp"
#include "ml/dataloaders/word2vec_loaders/sgns_w2v_dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/skip_gram.hpp"
#include "ml/ops/loss_functions/cross_entropy.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace fetch::ml;
using namespace fetch::ml::dataloaders;
using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using DataType     = double;
using ArrayType    = fetch::math::Tensor<DataType>;
using ArrayPtrType = std::shared_ptr<ArrayType>;
using SizeType     = typename ArrayType::SizeType;

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

void NormVector(ArrayType &vector)
{
  ArrayType::Type l2 = 0;
  for (auto &val : vector)
  {
    l2 += (val * val);
  }
  l2 = sqrt(l2);
  for (auto &val : vector)
  {
    val /= l2;
  }
}

void PrintWordAnology(W2VLoader<DataType> const &dl, ArrayType const &embeddings,
                      std::string const &word1, std::string const &word2, std::string const &word3,
                      SizeType k)
{
  ArrayType arr = embeddings;

  SizeType word1_idx = dl.IndexFromWord(word1);
  SizeType word2_idx = dl.IndexFromWord(word2);
  SizeType word3_idx = dl.IndexFromWord(word3);

  if (word1_idx == 0 || word2_idx == 0 || word3_idx == 0)
  {
    std::cout << "WARNING! not all to-be-tested words are in vocabulary" << std::endl;
  }
  else
  {
    std::cout << "Find word that to " << word3 << " is what " << word2 << " is to " << word1
              << std::endl;
  }

  ArrayType word1_vec = embeddings.Slice(word1_idx).Copy();
  ArrayType word2_vec = embeddings.Slice(word2_idx).Copy();
  ArrayType word3_vec = embeddings.Slice(word3_idx).Copy();

  NormVector(word1_vec);
  NormVector(word2_vec);
  NormVector(word3_vec);

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

void PrintKNN(W2VLoader<DataType> const &dl, ArrayType const &embeddings, std::string const &word0,
              SizeType k)
{
  ArrayType arr = embeddings;

  if (dl.IndexFromWord(word0) == 0)
  {
    std::cout << "WARNING! could not find [" + word0 + "] in vocabulary" << std::endl;
  }
  else
  {
    SizeType  idx        = dl.IndexFromWord(word0);
    ArrayType one_vector = embeddings.Slice(idx).Copy();
    std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> output =
        fetch::math::clustering::KNNCosine(arr, one_vector, k);

    for (std::size_t l = 0; l < output.size(); ++l)
    {
      std::cout << "rank: " << l << ", "
                << "distance, " << output.at(l).second << ": "
                << dl.WordFromIndex(output.at(l).first) << std::endl;
    }
  }
}

void PrintEmbedding(Graph<ArrayType> const &g, std::string const &skip_gram_name,
                    W2VLoader<DataType> const &dl, std::string word0)
{
  // first get hold of the skipgram layer by searching the return name in the graph
  std::shared_ptr<fetch::ml::layers::SkipGram<ArrayType>> sg_layer =
      std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<ArrayType>>(g.GetNode(skip_gram_name));

  // next get hold of the embeddings
  ArrayType embeddings = sg_layer->GetEmbeddings(sg_layer)->get_weights();

  if (dl.IndexFromWord(word0) == 0)
  {
    std::cout << "WARNING! could not find [" + word0 + "] in vocabulary" << std::endl;
  }
  else
  {
    SizeType  idx        = dl.IndexFromWord(word0);
    ArrayType one_vector = embeddings.Slice(idx).Copy();
    std::cout << "w2v vector: " << one_vector.ToString() << std::endl;
  }
}

void TestEmbeddings(Graph<ArrayType> const &g, std::string const &skip_gram_name,
                    W2VLoader<DataType> const &dl, std::string word0, std::string word1,
                    std::string word2, std::string word3, SizeType K)
{

  // first get hold of the skipgram layer by searching the return name in the graph
  std::shared_ptr<fetch::ml::layers::SkipGram<ArrayType>> sg_layer =
      std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<ArrayType>>(g.GetNode(skip_gram_name));

  // next get hold of the embeddings
  std::shared_ptr<fetch::ml::ops::Embeddings<ArrayType>> embeddings =
      sg_layer->GetEmbeddings(sg_layer);

  PrintKNN(dl, embeddings->get_weights(), word0, K);
  PrintWordAnology(dl, embeddings->get_weights(), word1, word2, word3, K);
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
  SizeType negative_sample_size = 20;    // number of negative sample per word-context pair
  SizeType window_size          = 8;     // window size for context sampling
  bool     train_mode           = true;  // reserve for future compatibility with CBOW

  SizeType min_count = 5;  // infrequent word removal threshold

  SizeType    output_size     = 1;
  SizeType    batch_size      = 128;      // training data batch size
  SizeType    embedding_size  = 32;       // dimension of embedding vec
  SizeType    training_epochs = 5;        // total number of training epochs
  double      learning_rate   = 0.1;      // alpha - the learning rate
  SizeType    k               = 10;       // how many nearest neighbours to compare against
  std::string word0           = "three";  // test word to consider
  std::string word1           = "France";
  std::string word2           = "Paris";
  std::string word3           = "Italy";
  std::string save_loc        = "./model.fba";  // save file location for exporting graph
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

  W2VLoader<DataType> data_loader(tp.window_size, tp.negative_sample_size, tp.train_mode);
  // set up dataloader
  /// DATA LOADING ///
  std::cout << "building vocab " << std::endl;
  data_loader.BuildVocab(ReadFile(train_file));
  data_loader.RemoveInfrequent(tp.min_count);
  data_loader.InitUnigramTable();
  std::cout << "Vocab Size : " << data_loader.vocab_size() << std::endl;

  ////////////////////////////////
  /// SETUP MODEL ARCHITECTURE ///
  ////////////////////////////////

  // set up model architecture
  std::cout << "building model architecture...: " << std::endl;
  std::shared_ptr<fetch::ml::Graph<ArrayType>> g(std::make_shared<fetch::ml::Graph<ArrayType>>());
  std::string model_name = Model(*g, tp.embedding_size, data_loader.vocab_size());

  // set up loss
  CrossEntropy<ArrayType> criterion;

  /////////////////////////////////
  /// TRAIN THE WORD EMBEDDINGS ///
  /////////////////////////////////

  std::cout << "beginning training...: " << std::endl;

  // Initialise Optimiser
  fetch::ml::optimisers::SGDOptimiser<ArrayType, fetch::ml::ops::CrossEntropy<ArrayType>> optimiser(
      g, {"Input", "Context"}, model_name, tp.learning_rate);

  // Training loop
  DataType loss;
  for (SizeType i{0}; i < tp.training_epochs; i++)
  {
    loss = optimiser.Run(data_loader, tp.batch_size, fetch::math::numeric_max<SizeType>());
    std::cout << "Loss: " << loss << std::endl;
    PrintEmbedding(*g, model_name, data_loader, tp.word0);
  }

  //////////////////////////////////////
  /// EXTRACT THE TRAINED EMBEDDINGS ///
  //////////////////////////////////////

  // Test trained embeddings
  TestEmbeddings(*g, model_name, data_loader, tp.word0, tp.word1, tp.word2, tp.word3, tp.k);

  return 0;
}
