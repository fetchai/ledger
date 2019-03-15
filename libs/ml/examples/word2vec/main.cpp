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
  SizeType batch_size     = 128;       // training data batch size
  SizeType embedding_size = 20;        // dimension of embedding vec
  SizeType training_steps = 12800000;  // total number of training steps
  double   learning_rate  = 0.0001;    // alpha - the learning rate
};

template <typename T>
SkipGramTextParams<T> SetParams()
{
  SkipGramTextParams<T> ret;

  ret.n_data_buffers = SizeType(2);   // input and context buffers
  ret.max_sentences  = SizeType(20);  // maximum number of sentences to use

  ret.unigram_table      = true;  // unigram table for sampling negative training pairs
  ret.unigram_table_size = SizeType(10000000);  // size of unigram table for negative sampling
  ret.unigram_power      = 0.75;                // adjusted unigram distribution

  ret.discard_frequent  = true;   // discard most frqeuent words
  ret.discard_threshold = 0.001;  // controls how aggressively to discard frequent words

  ret.window_size         = SizeType(5);   // max size of context window one way
  ret.min_sentence_length = SizeType(4);   // maximum number of sentences to use
  ret.k_negative_samples  = SizeType(15);  // number of negative examples to sample

  return ret;
}

std::string TRAINING_DATA = "/Users/khan/fetch/corpora/imdb_movie_review/aclImdb/train/unsup";

////////////////////////
/// MODEL DEFINITION ///
////////////////////////

std::string Model(fetch::ml::Graph<ArrayType> &g, SizeType vocab_size, SizeType embeddings_size)
{
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Context", {});
  std::string ret_name = g.AddNode<fetch::ml::layers::SkipGram<ArrayType>>(
      "SkipGram", {"Input", "Context"}, vocab_size, SizeType(1), embeddings_size);

  return ret_name;
}

std::vector<DataType> TestEmbeddings(Graph<ArrayType> &g, std::string &skip_gram_name,
                                     SkipGramLoader<ArrayType> &dl)
{

  // first get hold of the skipgram layer by searching the return name in the graph
  std::shared_ptr<fetch::ml::layers::SkipGram<ArrayType>> sg_layer =
      std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<ArrayType>>(g.GetNode(skip_gram_name));

  // next get hold of the embeddings
  std::shared_ptr<fetch::ml::ops::Embeddings<ArrayType>> embeddings =
      sg_layer->GetEmbeddings(sg_layer);

  // film
  ArrayType   embed_film_input(dl.VocabSize());
  std::string film_lookup       = "film";
  SizeType    film_idx          = dl.VocabLookup(film_lookup);
  embed_film_input.At(film_idx) = 1;
  ArrayType film_output         = embeddings->Forward({embed_film_input}).Clone();

  ArrayType   embed_movie_input(dl.VocabSize());
  std::string movie_lookup        = "movie";
  SizeType    movie_idx           = dl.VocabLookup(movie_lookup);
  embed_movie_input.At(movie_idx) = 1;
  ArrayType movie_output          = embeddings->Forward({embed_movie_input}).Clone();

  ArrayType   embed_great_input(dl.VocabSize());
  std::string great_lookup        = "great";
  SizeType    great_idx           = dl.VocabLookup(great_lookup);
  embed_great_input.At(great_idx) = 1;
  ArrayType great_output          = embeddings->Forward({embed_great_input}).Clone();

  ArrayType   embed_the_input(dl.VocabSize());
  std::string the_lookup      = "the";
  SizeType    the_idx         = dl.VocabLookup(the_lookup);
  embed_the_input.At(the_idx) = 1;
  ArrayType the_output        = embeddings->Forward({embed_the_input}).Clone();

  DataType result_film_movie, result_film_great, result_movie_great, result_movie_the;

  // distance from film to movie (using MSE as distance)
  result_film_movie = fetch::math::distance::Cosine(film_output, movie_output);

  // distance from film to great (using MSE as distance)
  result_film_great = fetch::math::distance::Cosine(film_output, great_output);

  // distance from movie to great (using MSE as distance)
  result_movie_great = fetch::math::distance::Cosine(movie_output, great_output);

  // distance from movie to the (using MSE as distance)
  result_movie_the = fetch::math::distance::Cosine(movie_output, the_output);

  std::vector<DataType> ret = {result_film_movie, result_film_great, result_movie_great,
                               result_movie_the};

  std::cout << "film-movie distance: " << ret[0] << std::endl;
  std::cout << "film-great distance: " << ret[1] << std::endl;
  std::cout << "movie-great distance: " << ret[2] << std::endl;
  std::cout << "movie-the distance: " << ret[3] << std::endl;

  return ret;
}

int main()
{

  std::cout << "FETCH Word2Vec Demo" << std::endl;

  TrainingParams                tp;
  SkipGramTextParams<ArrayType> sp = SetParams<ArrayType>();

  ///////////////////////////////////////
  /// CONVERT TEXT INTO TRAINING DATA ///
  ///////////////////////////////////////

  // set up dataloader
  std::cout << "Setting up training data...: " << std::endl;
  SkipGramLoader<ArrayType> dataloader(TRAINING_DATA, sp);

  ////////////////////////////////
  /// SETUP MODEL ARCHITECTURE ///
  ////////////////////////////////

  // set up model architecture
  std::cout << "building model architecture...: " << std::endl;
  fetch::ml::Graph<ArrayType> g;
  std::string                 output_name = Model(g, dataloader.VocabSize(), tp.embedding_size);

  // set up loss
  ScaledCrossEntropy<ArrayType> criterion;

  /////////////////////////////////
  /// TRAIN THE WORD EMBEDDINGS ///
  /////////////////////////////////

  std::cout << "beginning training...: " << std::endl;

  std::pair<ArrayType, SizeType> data;
  ArrayType                      input(std::vector<typename ArrayType::SizeType>({1, 1}));
  ArrayType                      context(std::vector<typename ArrayType::SizeType>({1, 1}));
  ArrayType                      gt(std::vector<typename ArrayType::SizeType>({1, 1}));
  DataType                       loss = 0;
  ArrayType                      scale_factor(std::vector<typename ArrayType::SizeType>({1, 1}));

  for (std::size_t i = 0; i < tp.training_steps; ++i)
  {
    // get random data point
    data = dataloader.GetRandom();

    // assign input and context vectors
    SizeType count_idx = 0;
    for (auto &e : data.first)
    {
      if (count_idx < dataloader.VocabSize())
      {
        input.At(count_idx) = e;
      }
      else
      {
        context.At(count_idx - dataloader.VocabSize()) = e;
      }
      ++count_idx;
    }
    g.SetInput("Input", input);
    g.SetInput("Context", context);

    // assign label
    gt.Fill(0);
    gt.At(0) = DataType(data.second);
    //    gt->At(SizeType(data.second)) = DataType(1);

    // forward pass
    ArrayType results = g.Evaluate(output_name);

    //    // result interpreted as probability True - so reverse for gt == 0

    //    std::cout << "gt->At(0): " << gt->At(0) << std::endl;
    //    std::cout << "results->At(0): " << results->At(0) << std::endl;
    if (gt.At(0) == DataType(0))
    {
      results.At(0)      = DataType(1) - results.At(0);
      scale_factor.At(0) = DataType(sp.k_negative_samples);
    }
    else
    {
      scale_factor.At(0) = DataType(1);
    }
    //    std::cout << "results->At(0): " << results->At(0) << std::endl;

    // cost function
    DataType tmp_loss = criterion.Forward({results, gt, scale_factor});

    // diminish size of updates due to negative examples
    if (data.second == 0)
    {
      tmp_loss /= DataType(sp.k_negative_samples);
    }

    loss += tmp_loss;
    //
    //    std::cout << "gt->At(0: " << gt->At(0) << std::endl;
    //    std::cout << "results->At(0): " << results->At(0) << std::endl;
    //    std::cout << "tmp_loss: " << tmp_loss << std::endl;

    // backprop
    g.BackPropagate(output_name, criterion.Backward(std::vector<ArrayType>({results, gt})));

    if (i % tp.batch_size == (tp.batch_size - 1))
    {
      std::cout << "MiniBatch: " << i / tp.batch_size << " -- Loss : " << loss << std::endl;
      g.Step(tp.learning_rate);
      loss = 0;
    }

    if (i % (tp.batch_size * 100) == ((tp.batch_size * 100) - 1))
    {
      // Test trained embeddings
      std::vector<DataType> trained_distances = TestEmbeddings(g, output_name, dataloader);
    }
  }

  //////////////////////////////////////
  /// EXTRACT THE TRAINED EMBEDDINGS ///
  //////////////////////////////////////

  // Test trained embeddings
  std::vector<DataType> trained_distances = TestEmbeddings(g, output_name, dataloader);

  std::cout << "final film-movie distance: " << trained_distances[0] << std::endl;
  std::cout << "final film-great distance: " << trained_distances[1] << std::endl;
  std::cout << "final movie-great distance: " << trained_distances[2] << std::endl;
  std::cout << "final movie-the distance: " << trained_distances[3] << std::endl;

  return 0;
}
