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

//#include "math/tensor.hpp"
//#include "ml/ops/activation.hpp"
#include "ml/dataloaders/dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/skip_gram.hpp"
#include "ml/ops/loss_functions/cross_entropy.hpp"
#include "w2v_dataloader.hpp"

#include "math/free_functions/ml/loss_functions/mean_square_error.hpp"

#include <iostream>
#include <math/tensor.hpp>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using DataType  = float;
using ArrayType = fetch::math::Tensor<DataType>;
using SizeType  = typename ArrayType::SizeType;

////////////////////////////////
/// PARAMETERS AND CONSTANTS ///
////////////////////////////////

//#define MAX_STRING 100
//#define EXP_TABLE_SIZE 1000
//#define MAX_EXP 6
//#define MAX_SENTENCE_LENGTH 1000
//#define MAX_CODE_LENGTH 40

struct PARAMS
{
  bool     cbow       = false;  // skipgram model if false, cbow if true
  SizeType batch_size = 1;      // training data batch size
  SizeType embedding_size =
      3;                     // dimension of the embedding vector - 4th root of vocab size is good
  SizeType skip_window = 2;  // words to include in frame (left & right)
  //  std::uint64_t num_skips      = 2;      // n times to reuse an input to generate a label
  SizeType k_neg_samps    = 2;    // number of negative examples to sample
  double   discard_thresh = 0.9;  // larger the training data set, lower the discard threshold
  SizeType training_steps = 10000;
};

std::string TRAINING_DATA =
    "A cat is not a dog.\n"
    "But a cat is sort of related to a dog in some ways.\n"
    "For example they are both animals.\n"
    "Unlike a car which is not an animal.\n"
    "A car is a machine.\n"
    "Machines ares types of automated tools, like a computer or a hammer.\n"
    "Word2vec is also a tool; but it is software.\n"
    "Software is a tool that you cannot touch.";

////////////////////////
/// MODEL DEFINITION ///
////////////////////////

std::string Model(fetch::ml::Graph<ArrayType> &g, SizeType vocab_size, SizeType batch_size,
                  SizeType embeddings_size)
{
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Context", {});
  std::string ret_name = g.AddNode<fetch::ml::layers::SkipGram<ArrayType>>(
      "SkipGram", {"Input", "Context"}, vocab_size, SizeType(2), embeddings_size);

  return ret_name;
}

void TestEmbeddings(fetch::ml::Graph<ArrayType> &g, std::string &skip_gram_name,
                    fetch::ml::W2VLoader<ArrayType> &dl)
{

  // first get hold of the skipgram layer by searching the return name in the graph
  std::shared_ptr<fetch::ml::layers::SkipGram<ArrayType>> sg_layer =
      std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<ArrayType>>(g.GetNode(skip_gram_name));

  // next get hold of the embeddings
  std::shared_ptr<ArrayType> embeddings = sg_layer->GetEmbeddings();

  std::cout << "embeddings->At(0): " << embeddings->At(0) << std::endl;

  // embeddings
  std::cout << "embeddings dimensions: " << embeddings->shape()[1] << std::endl;
  std::cout << "vocab size: " << embeddings->shape()[0] << std::endl;

  // cat
  std::string lookup_word = "cat";
  SizeType    cat_idx     = dl.VocabLookup(lookup_word);
  for (std::size_t j = 0; j < embeddings->shape()[1]; ++j)
  {
    std::cout << "embeddings->At({cat_idx, j}): " << embeddings->At({cat_idx, j}) << std::endl;
  }
  std::cout << "\n " << std::endl;

  // dog
  lookup_word      = "dog";
  SizeType dog_idx = dl.VocabLookup(lookup_word);
  for (std::size_t j = 0; j < embeddings->shape()[1]; ++j)
  {
    std::cout << "embeddings->At({dog_idx, j}): " << embeddings->At({dog_idx, j}) << std::endl;
  }
  std::cout << "\n " << std::endl;

  // computer
  lookup_word           = "computer";
  SizeType computer_idx = dl.VocabLookup(lookup_word);
  for (std::size_t j = 0; j < embeddings->shape()[1]; ++j)
  {
    std::cout << "embeddings->At({computer_idx, j}): " << embeddings->At({computer_idx, j})
              << std::endl;
  }

  DataType result;
  // distance from cat to dog (using MSE as distance)
  result = fetch::math::MeanSquareError(embeddings->Slice(cat_idx), embeddings->Slice(dog_idx));
  std::cout << "distance from cat to dog: " << result << std::endl;

  // distance from cat to computer (using MSE as distance)
  result =
      fetch::math::MeanSquareError(embeddings->Slice(cat_idx), embeddings->Slice(computer_idx));
  std::cout << "distance from cat to computer: " << result << std::endl;
}

int main(int ac, char **av)
{

  std::cout << "FETCH Word2Vec Demo" << std::endl;

  PARAMS p;

  // set up dataloader
  std::cout << "Setting up training data...: " << std::endl;
  fetch::ml::W2VLoader<ArrayType> dataloader(TRAINING_DATA, p.skip_window, p.cbow, p.k_neg_samps,
                                             p.discard_thresh);

  // set up model architecture
  std::cout << "building model architecture...: " << std::endl;
  fetch::ml::Graph<ArrayType> g;
  std::string output_name = Model(g, dataloader.VocabSize(), p.batch_size, p.embedding_size);

  // Test untrained embeddings
  TestEmbeddings(g, output_name, dataloader);

  // set up loss
  CrossEntropy<ArrayType> criterion;

  std::pair<std::pair<std::shared_ptr<ArrayType>, std::shared_ptr<ArrayType>>, SizeType> input;
  std::shared_ptr<ArrayType>                                                             gt =
      std::make_shared<ArrayType>(std::vector<typename ArrayType::SizeType>({1, 2}));
  DataType loss = 0;

  /////////////////////////////////
  /// TRAIN THE WORD EMBEDDINGS ///
  /////////////////////////////////

  std::cout << "beginning training...: " << std::endl;

  for (std::size_t i = 0; i < p.training_steps; ++i)
  {
    input = dataloader.GetRandom();

    g.SetInput("Input", input.first.first);
    g.SetInput("Context", input.first.second);
    gt->Fill(0);
    gt->At(input.second)               = DataType(1);
    std::shared_ptr<ArrayType> results = g.Evaluate(output_name);

    loss += criterion.Forward({results, gt});

    g.BackPropagate(output_name, criterion.Backward({results, gt}));

    if (i % 50 == 0)
    {
      std::cout << "MiniBatch: " << i / 50 << " -- Loss : " << loss << std::endl;
      g.Step(0.01f);
      loss = 0;
    }
  }

  //////////////////////////////////////
  /// EXTRACT THE TRAINED EMBEDDINGS ///
  //////////////////////////////////////

  // Test untrained embeddings
  TestEmbeddings(g, output_name, dataloader);

  return 0;
}
