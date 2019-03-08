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
  SizeType k_neg_samps    = 2;  // number of negative examples to sample
  SizeType training_steps = 50000;
};

std::string TRAINING_DATA =
    "Word2vec is a group of related models that are used to produce word embeddings. "
    "These models are shallow, two-layer neural networks that are trained to reconstruct "
    "linguistic "
    "contexts of words. Word2vec takes as its input a large corpus of text and produces a vector "
    "space, "
    "typically of several hundred dimensions, with each unique word in the corpus being assigned a "
    "corresponding vector in the space. Word vectors are positioned in the vector space such that "
    "words that share common contexts in the corpus are located in close proximity to one another "
    "in "
    "the space."
    "Word2vec was created by a team of researchers led by Tomáš Mikolov at Google and patented. "
    "The "
    "algorithm has been subsequently analysed and explained by other researchers. Embedding "
    "vectors "
    "created using the Word2vec algorithm have many advantages compared to earlier algorithms such "
    "as latent semantic analysis.";

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

int main(int ac, char **av)
{

  std::cout << "FETCH Word2Vec Demo" << std::endl;

  PARAMS p;

  // set up dataloader
  std::cout << "Setting up training data...: " << std::endl;
  fetch::ml::W2VLoader<ArrayType> dataloader(TRAINING_DATA, p.skip_window, p.cbow, p.k_neg_samps);

  // set up model architecture
  std::cout << "building model architecture...: " << std::endl;
  fetch::ml::Graph<ArrayType> g;
  std::string output_name = Model(g, dataloader.VocabSize(), p.batch_size, p.embedding_size);

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
//
//    std::cout << "results->At(0): " << results->At(0) << std::endl;
//    std::cout << "results->At(1): " << results->At(1) << std::endl;
//    std::cout << "gt->At(0): " << gt->At(0) << std::endl;
//    std::cout << "gt->At(1): " << gt->At(1) << std::endl;

    loss += criterion.Forward({results, gt});
//    std::cout << "loss: " << loss << std::endl;

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

  // first get hold of the skipgram layer by searching the return name in the graph
  std::shared_ptr<fetch::ml::layers::SkipGram<ArrayType>> sg_layer =
      std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<ArrayType>>(g.GetNode(output_name));

  // next get hold of the embeddings
  std::shared_ptr<ArrayType> embeddings = sg_layer->GetEmbeddings();

  // embeddings
  std::cout << "embeddings dimensions: " << embeddings->shape()[1] << std::endl;
  std::cout << "vocab size: " << embeddings->shape()[0] << std::endl;
  //
  //  dataloader.Size()

  return 0;
}
