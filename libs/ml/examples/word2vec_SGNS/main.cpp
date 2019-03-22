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

#include "math/free_functions/clustering_algorithms/knn.hpp"

#include "ml/graph.hpp"
#include "ml/layers/skip_gram.hpp"
#include "ml/ops/loss_functions/scaled_cross_entropy.hpp"
#include "ml/dataloaders/word2vec_loaders/skipgram_dataloader.hpp"
#include "file_loader.hpp"

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
  SizeType output_size    = 2;
  SizeType batch_size     = 1;         // training data batch size
  SizeType embedding_size = 16;         // dimension of embedding vec
  SizeType training_epochs = 1000;      // total number of training epochs
  double   learning_rate  = 0.01;      // alpha - the learning rate
};

template <typename T>
SkipGramTextParams<T> SetParams()
{
  SkipGramTextParams<T> ret;

  ret.n_data_buffers = SizeType(2);    // input and context buffers
  ret.max_sentences  = SizeType(100);  // maximum number of sentences to use

  ret.unigram_table      = true;  // unigram table for sampling negative training pairs
  ret.unigram_table_size = SizeType(10000000);  // size of unigram table for negative sampling
  ret.unigram_power      = 0.75;                // adjusted unigram distribution

  ret.discard_frequent  = true;   // discard most frqeuent words
  ret.discard_threshold = 0.005;  // controls how aggressively to discard frequent words

  ret.window_size         = SizeType(5);   // max size of context window one way
  ret.min_sentence_length = SizeType(4);   // maximum number of sentences to use
  ret.k_negative_samples  = SizeType(5);  // number of negative examples to sample

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
      "SkipGram", {"Input", "Context"}, SizeType(1), SizeType(2), embeddings_size, vocab_size);

  return ret_name;
}


void PrintKNN(SkipGramLoader<ArrayType> const &dl, ArrayType const &embeddings, std::string const &word, unsigned int k)
{
  ArrayType arr = embeddings;
  ArrayType one_vector = embeddings.Slice(dl.VocabLookup(word)).Unsqueeze();
  std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> output = fetch::math::clustering::KNN(arr, one_vector, k);

  for (std::size_t j = 0; j < output.size(); ++j)
  {
    std::cout << "output.at(j).first: " << dl.VocabLookup(output.at(j).first) << std::endl;
    std::cout << "output.at(j).second: " << output.at(j).second << "\n" << std::endl;
  }

  std::cout << "hot-cold distance: " << fetch::math::distance::Cosine(embeddings.Slice(dl.VocabLookup("cold")).Unsqueeze(), embeddings.Slice(dl.VocabLookup("hot")).Unsqueeze()) << std::endl;
}

void TestEmbeddings(Graph<ArrayType> const &g, std::string const &skip_gram_name,
                    SkipGramLoader<ArrayType> const &dl)
{

  // first get hold of the skipgram layer by searching the return name in the graph
  std::shared_ptr<fetch::ml::layers::SkipGram<ArrayType>> sg_layer =
      std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<ArrayType>>(g.GetNode(skip_gram_name));

  // next get hold of the embeddings
  std::shared_ptr<fetch::ml::ops::Embeddings<ArrayType>> embeddings =
      sg_layer->GetEmbeddings(sg_layer);

  PrintKNN(dl, embeddings->GetWeights(), "cold", 10);
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

  std::cout << "Setting up training data...: " << std::endl;

  // set up dataloader
  SkipGramLoader<ArrayType> dataloader(sp);

  // load text from files as necessary and process text with dataloader
  dataloader.AddData(GetTextString(training_text));

  std::cout << "dataloader.VocabSize(): " << dataloader.VocabSize() << std::endl;
  std::cout << "dataloader.Size(): " << dataloader.Size() << std::endl;
  
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
  ArrayType gt(std::vector<typename ArrayType::SizeType>({tp.batch_size, tp.output_size}));
  DataType  loss = 0;
  ArrayType scale_factor(std::vector<typename ArrayType::SizeType>({tp.batch_size, 1}));

  for (std::size_t i = 0; i < tp.training_epochs; ++i)
  {
    double epoch_loss = 0;
    SizeType step_count = 0;
    while (!dataloader.IsDone())
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
        gt.At(data.second) = DataType(1);
      }

      g.SetInput("Input", input, false);
      g.SetInput("Context", context, false);

      // forward pass
      ArrayType results = g.Evaluate(output_name);

      int pos_count = 0;
      int neg_count = 0;
      for (std::size_t j = 0; j < tp.batch_size; ++j)
      {
        if (gt.At({j, 0}) == DataType(1))
        {
          scale_factor.At(j) = DataType(sp.k_negative_samples);
          neg_count++;
        }
        else
        {
          scale_factor.At(j) = DataType(1);
          pos_count++;
        }
      }

      // cost function
      DataType tmp_loss = criterion.Forward({results, gt, scale_factor});
      // diminish size of updates due to negative examples
      if (data.second == 0)
      {
        tmp_loss /= DataType(sp.k_negative_samples);
      }
      loss += tmp_loss;

      // backprop
      g.BackPropagate(output_name, criterion.Backward(std::vector<ArrayType>({results, gt})));

      // take mini-batch learning step
      if (step_count % 32 == (32 - 1))
      {
        std::cout << "MiniBatch: " << step_count / 32 << " -- Loss : " << loss << std::endl;
        g.Step(tp.learning_rate);
        epoch_loss += loss;
        loss = 0;
      }
      ++step_count;
    }
    dataloader.Reset();

    // print batch loss and embeddings distances
    // Test trained embeddings
    TestEmbeddings(g, output_name, dataloader);
    std::cout << "epoch_loss: " << epoch_loss << std::endl;
  }

  //////////////////////////////////////
  /// EXTRACT THE TRAINED EMBEDDINGS ///
  //////////////////////////////////////

  // Test trained embeddings
  TestEmbeddings(g, output_name, dataloader);

  return 0;
}
