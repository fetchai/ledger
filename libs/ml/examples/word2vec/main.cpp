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
#include "ml/dataloaders/w2v_dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/skip_gram.hpp"
#include "ml/ops/loss_functions/scaled_cross_entropy.hpp"

#include <iostream>
#include <math/tensor.hpp>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;
using DataType     = double;
using ArrayType    = fetch::math::Tensor<DataType>;
using ArrayPtrType = std::shared_ptr<ArrayType>;
using SizeType     = typename ArrayType::SizeType;

////////////////////////////////
/// PARAMETERS AND CONSTANTS ///
////////////////////////////////

struct PARAMS
{
  SizeType batch_size     = 128;       // training data batch size
  SizeType embedding_size = 20;       // dimension of embedding vec
  SizeType training_steps = 12800000;  // total number of training steps
  double   learning_rate  = 0.0001;    // alpha - the learning rate
  bool     cbow           = false;     // skipgram model if false, cbow if true
  SizeType skip_window    = 5;         // max size of context window one way
  SizeType super_samp     = 1;         // n times to reuse an input to generate a label
  SizeType k_neg_samps    = 15;        // number of negative examples to sample
  double   discard_thresh = 0.001;     // controls how aggressively to discard frequent words
  SizeType max_sentences  = 20;       // maximum number of sentences to use
};

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

std::vector<DataType> TestEmbeddings(fetch::ml::Graph<ArrayType> &g, std::string &skip_gram_name,
                                     fetch::ml::W2VLoader<ArrayType> &dl)
{

  // first get hold of the skipgram layer by searching the return name in the graph
  std::shared_ptr<fetch::ml::layers::SkipGram<ArrayType>> sg_layer =
      std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<ArrayType>>(g.GetNode(skip_gram_name));

  // next get hold of the embeddings
  std::shared_ptr<fetch::ml::ops::Embeddings<ArrayType>> embeddings =
      sg_layer->GetEmbeddings(sg_layer);

  // film
  ArrayPtrType embed_film_input  = std::make_shared<ArrayType>(dl.VocabSize());
  std::string  film_lookup       = "film";
  SizeType     film_idx          = dl.VocabLookup(film_lookup);
  embed_film_input->At(film_idx) = 1;
  ArrayType film_output          = embeddings->Forward({embed_film_input})->Clone();

  ArrayPtrType embed_movie_input   = std::make_shared<ArrayType>(dl.VocabSize());
  std::string  movie_lookup        = "movie";
  SizeType     movie_idx           = dl.VocabLookup(movie_lookup);
  embed_movie_input->At(movie_idx) = 1;
  ArrayType movie_output           = embeddings->Forward({embed_movie_input})->Clone();

  ArrayPtrType embed_great_input   = std::make_shared<ArrayType>(dl.VocabSize());
  std::string  great_lookup        = "great";
  SizeType     great_idx           = dl.VocabLookup(great_lookup);
  embed_great_input->At(great_idx) = 1;
  ArrayType great_output           = embeddings->Forward({embed_great_input})->Clone();

  ArrayPtrType embed_the_input = std::make_shared<ArrayType>(dl.VocabSize());
  std::string  the_lookup      = "the";
  SizeType     the_idx         = dl.VocabLookup(the_lookup);
  embed_the_input->At(the_idx) = 1;
  ArrayType the_output         = embeddings->Forward({embed_the_input})->Clone();

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

  PARAMS p;

  ///////////////////////////////////////
  /// CONVERT TEXT INTO TRAINING DATA ///
  ///////////////////////////////////////

  // set up dataloader
  std::cout << "Setting up training data...: " << std::endl;
  fetch::ml::W2VLoader<ArrayType> dataloader(TRAINING_DATA, p.cbow, p.skip_window, p.super_samp,
                                             p.k_neg_samps, p.discard_thresh, p.max_sentences);

  std::cout << "dataloader.VocabSize(): " << dataloader.VocabSize() << std::endl;

  ////////////////////////////////
  /// SETUP MODEL ARCHITECTURE ///
  ////////////////////////////////

  // set up model architecture
  std::cout << "building model architecture...: " << std::endl;
  fetch::ml::Graph<ArrayType> g;
  std::string                 output_name = Model(g, dataloader.VocabSize(), p.embedding_size);

  // set up loss
  ScaledCrossEntropy<ArrayType> criterion;

  /////////////////////////////////
  /// TRAIN THE WORD EMBEDDINGS ///
  /////////////////////////////////

  std::cout << "beginning training...: " << std::endl;

  std::pair<std::shared_ptr<ArrayType>, SizeType> data;
  std::shared_ptr<ArrayType>                      input = std::make_shared<ArrayType>(
      std::vector<typename ArrayType::SizeType>({1, dataloader.VocabSize()}));
  ;
  std::shared_ptr<ArrayType> context = std::make_shared<ArrayType>(
      std::vector<typename ArrayType::SizeType>({1, dataloader.VocabSize()}));
  std::shared_ptr<ArrayType> gt =
      std::make_shared<ArrayType>(std::vector<typename ArrayType::SizeType>({1, 1}));
  DataType                   loss = 0;
  std::shared_ptr<ArrayType> scale_factor =
      std::make_shared<ArrayType>(std::vector<typename ArrayType::SizeType>({1, 1}));

  for (std::size_t i = 0; i < p.training_steps; ++i)
  {
    // get random data point
    data = dataloader.GetRandom();

    // assign input and context vectors
    SizeType count_idx = 0;
    for (auto &e : *(data.first))
    {
      if (count_idx < dataloader.VocabSize())
      {
        input->At(count_idx) = e;
      }
      else
      {
        context->At(count_idx - dataloader.VocabSize()) = e;
      }
      ++count_idx;
    }
    g.SetInput("Input", input);
    g.SetInput("Context", context);

    // assign label
    gt->Fill(0);
    gt->At(0) = DataType(data.second);
    //    gt->At(SizeType(data.second)) = DataType(1);

    // forward pass
    std::shared_ptr<ArrayType> results = g.Evaluate(output_name);


    //    // result interpreted as probability True - so reverse for gt == 0

    std::cout << "gt->At(0): " << gt->At(0) << std::endl;
    std::cout << "results->At(0): " << results->At(0) << std::endl;
    if (gt->At(0) == DataType(0))
    {
      results->At(0) = DataType(1) - results->At(0);
      scale_factor->At(0) = DataType(p.k_neg_samps * 2);
    }
    else
    {
      scale_factor->At(0) = DataType(1);
    }
    std::cout << "results->At(0): " << results->At(0) << std::endl;

    // cost function
    DataType tmp_loss = criterion.Forward({results, gt, scale_factor});

    // diminish size of updates due to negative examples
    if (data.second == 0)
    {
      tmp_loss /= DataType(p.k_neg_samps);
    }

    loss += tmp_loss;

    std::cout << "gt->At(0: " << gt->At(0) << std::endl;
    std::cout << "results->At(0): " << results->At(0) << std::endl;
    std::cout << "tmp_loss: " << tmp_loss << std::endl;

    // backprop
    g.BackPropagate(output_name, criterion.Backward({results, gt}));

    if (i % p.batch_size == (p.batch_size - 1))
    {
      std::cout << "MiniBatch: " << i / p.batch_size << " -- Loss : " << loss << std::endl;
      g.Step(p.learning_rate);
      loss = 0;
    }

    if (i % (p.batch_size * 100) == ((p.batch_size * 100) - 1))
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
