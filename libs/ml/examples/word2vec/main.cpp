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

#include "math/distance/cosine.hpp"
#include "ml/dataloaders/skipgram_dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/skip_gram.hpp"
#include "ml/ops/loss_functions/scaled_cross_entropy.hpp"

#include <iostream>
#include <math/tensor.hpp>

using namespace fetch::ml;
using namespace fetch::ml::dataloaders;
using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using DataType     = double;
using ArrayType    = fetch::math::Tensor<DataType>;
using ArrayPtrType = std::shared_ptr<ArrayType>;
using SizeType     = typename ArrayType::SizeType;

////////////////////////////////
/// PARAMETERS AND CONSTANTS ///
////////////////////////////////

struct TrainingParams
{
  SizeType batch_size     = 1;         // training data batch size
  SizeType embedding_size = 128;       // dimension of embedding vec
  SizeType training_steps = 12800000;  // total number of training steps
  double   learning_rate  = 0.001;     // alpha - the learning rate
};

template <typename T>
SkipGramTextParams<T> SetParams()
{
  SkipGramTextParams<T> ret;

  ret.n_data_buffers = SizeType(2);       // input and context buffers
  ret.max_sentences  = SizeType(100000);  // maximum number of sentences to use

  ret.unigram_table      = true;  // unigram table for sampling negative training pairs
  ret.unigram_table_size = SizeType(10000000);  // size of unigram table for negative sampling
  ret.unigram_power      = 0.75;                // adjusted unigram distribution

  ret.discard_frequent  = true;    // discard most frqeuent words
  ret.discard_threshold = 0.0001;  // controls how aggressively to discard frequent words

  ret.window_size         = SizeType(5);  // max size of context window one way
  ret.min_sentence_length = SizeType(4);  // maximum number of sentences to use
  ret.k_negative_samples  = SizeType(1);  // number of negative examples to sample

  return ret;
}

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

std::vector<DataType> TestEmbeddings(Graph<ArrayType> const &g, std::string const &skip_gram_name,
                                     SkipGramLoader<ArrayType> &dl)
{

  // first get hold of the skipgram layer by searching the return name in the graph
  std::shared_ptr<fetch::ml::layers::SkipGram<ArrayType>> sg_layer =
      std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<ArrayType>>(g.GetNode(skip_gram_name));

  // next get hold of the embeddings
  std::shared_ptr<fetch::ml::ops::Embeddings<ArrayType>> embeddings =
      sg_layer->GetEmbeddings(sg_layer);

  // hollywood
  ArrayType   embed_hollywood_input(1);
  std::string hollywood_lookup = "hollywood";
  SizeType    hollywood_idx    = dl.VocabLookup(hollywood_lookup);
  embed_hollywood_input.At(0)  = DataType(hollywood_idx);
  ArrayType hollywood_output   = embeddings->Forward({embed_hollywood_input}).Clone();

  ArrayType   embed_movie_input(1);
  std::string movie_lookup = "movie";
  SizeType    movie_idx    = dl.VocabLookup(movie_lookup);
  embed_movie_input.At(0)  = DataType(movie_idx);
  ArrayType movie_output   = embeddings->Forward({embed_movie_input}).Clone();

  ArrayType   embed_husband_input(1);
  std::string husband_lookup = "husband";
  SizeType    husband_idx    = dl.VocabLookup(husband_lookup);
  embed_husband_input.At(0)  = DataType(husband_idx);
  ArrayType husband_output   = embeddings->Forward({embed_husband_input}).Clone();

  ArrayType   embed_wife_input(1);
  std::string wife_lookup = "wife";
  SizeType    wife_idx    = dl.VocabLookup(wife_lookup);
  embed_wife_input.At(0)  = DataType(wife_idx);
  ArrayType wife_output   = embeddings->Forward({embed_wife_input}).Clone();

  DataType result_hollywood_movie, result_hollywood_husband, result_movie_husband,
      result_husband_wife;

  // distance from hollywood to movie (using MSE as distance)
  result_hollywood_movie = fetch::math::distance::Cosine(hollywood_output, movie_output);

  // distance from hollywood to husband (using MSE as distance)
  result_hollywood_husband = fetch::math::distance::Cosine(hollywood_output, husband_output);

  // distance from movie to husband (using MSE as distance)
  result_movie_husband = fetch::math::distance::Cosine(movie_output, husband_output);

  // distance from movie to the (using MSE as distance)
  result_husband_wife = fetch::math::distance::Cosine(husband_output, wife_output);

  std::vector<DataType> ret = {result_hollywood_movie, result_hollywood_husband,
                               result_movie_husband, result_husband_wife};

  std::cout << "hollywood-movie distance: " << ret[0] << std::endl;
  std::cout << "hollywood-husband distance: " << ret[1] << std::endl;
  std::cout << "movie-husband distance: " << ret[2] << std::endl;
  std::cout << "husband-wife distance: " << ret[3] << std::endl;

  return ret;
}

int main(int argc, char **argv)
{

  std::string training_text;
  if (argc == 2)
  {
    training_text = argv[1];
  }
  else
  {
    throw std::runtime_error("must specify filename as training text");
  }

  std::cout << "FETCH Word2Vec Demo" << std::endl;

  TrainingParams                tp;
  SkipGramTextParams<ArrayType> sp = SetParams<ArrayType>();

  ///////////////////////////////////////
  /// CONVERT TEXT INTO TRAINING DATA ///
  ///////////////////////////////////////

  // set up dataloader
  std::cout << "Setting up training data...: " << std::endl;
  SkipGramLoader<ArrayType> dataloader(training_text, sp);

  ////////////////////////////////
  /// SETUP MODEL ARCHITECTURE ///
  ////////////////////////////////

  // set up model architecture
  std::cout << "building model architecture...: " << std::endl;
  fetch::ml::Graph<ArrayType> g;
  std::string                 output_name = Model(g, tp.embedding_size, dataloader.VocabSize());

  // set up loss
  ScaledCrossEntropy<ArrayType> criterion;

  /////////////////////////////////
  /// TRAIN THE WORD EMBEDDINGS ///
  /////////////////////////////////

  std::cout << "beginning training...: " << std::endl;

  std::pair<ArrayType, SizeType> data;
  ArrayType input(std::vector<typename ArrayType::SizeType>({tp.batch_size, 1}));
  ArrayType context(std::vector<typename ArrayType::SizeType>({tp.batch_size, 1}));
  ArrayType gt(std::vector<typename ArrayType::SizeType>({tp.batch_size, 1}));
  DataType  loss = 0;
  ArrayType scale_factor(std::vector<typename ArrayType::SizeType>({tp.batch_size, 1}));

  ArrayType squeezed_result({tp.batch_size, 1});

  double batch_loss = 0;
  for (std::size_t i = 0; i < tp.training_steps; ++i)
  {
    gt.Fill(0);
    for (std::size_t j = 0; j < tp.batch_size; ++j)
    {
      // get random data point
      data = dataloader.GetRandom();

      // assign input and context vectors
      input.At(j)   = data.first.At(0);
      context.At(j) = data.first.At(1);

      // assign label
      gt.At(j) = DataType(data.second);
    }

    g.SetInput("Input", input, false);
    g.SetInput("Context", context, false);

    // forward pass
    ArrayType results = g.Evaluate(output_name);

    int pos_count = 0;
    int neg_count = 0;
    for (std::size_t j = 0; j < tp.batch_size; ++j)
    {
      // result interpreted as probability True - so reverse for gt == 0
      if (gt.At(j) == DataType(0))
      {
        results.At(j)      = DataType(1) - results.At(j);
        scale_factor.At(j) = DataType(sp.k_negative_samples);
        neg_count++;
      }
      else
      {
        scale_factor.At(j) = DataType(1);
        pos_count++;
      }
      squeezed_result.At(j) = results.At(j);
    }

    // cost function
    DataType tmp_loss = criterion.Forward({squeezed_result, gt, scale_factor});
    // diminish size of updates due to negative examples
    if (data.second == 0)
    {
      tmp_loss /= DataType(sp.k_negative_samples);
    }
    loss += tmp_loss;

    // backprop
    g.BackPropagate(output_name, criterion.Backward(std::vector<ArrayType>({squeezed_result, gt})));

    // take mini-batch learning step
    if (i % tp.batch_size == (tp.batch_size - 1))
    {
      std::cout << "MiniBatch: " << i / tp.batch_size << " -- Loss : " << loss << std::endl;
      g.Step(tp.learning_rate);
      batch_loss += loss;
      loss = 0;
    }

    // print batch loss and embeddings distances
    if (i % (tp.batch_size * 100) == ((tp.batch_size * 100) - 1))
    {
      // Test trained embeddings
      std::vector<DataType> trained_distances = TestEmbeddings(g, output_name, dataloader);
      std::cout << "batch_loss: " << batch_loss << std::endl;
      batch_loss = 0;
    }
  }

  //////////////////////////////////////
  /// EXTRACT THE TRAINED EMBEDDINGS ///
  //////////////////////////////////////

  // Test trained embeddings
  std::vector<DataType> trained_distances = TestEmbeddings(g, output_name, dataloader);

  std::cout << "final hollywood-movie distance: " << trained_distances[0] << std::endl;
  std::cout << "final hollywood-husband distance: " << trained_distances[1] << std::endl;
  std::cout << "final movie-husband distance: " << trained_distances[2] << std::endl;
  std::cout << "final husband-wife distance: " << trained_distances[3] << std::endl;

  return 0;
}
