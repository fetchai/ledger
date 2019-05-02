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

#include "ml/dataloaders/word2vec_loaders/skipgram_dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/skip_gram.hpp"
#include "ml/ops/loss_functions/cross_entropy.hpp"

#include <chrono>
#include <iostream>
#include <math/tensor.hpp>
#include <numeric>

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
  SizeType    output_size     = 1;
  SizeType    batch_size      = 500;            // training data batch size
  SizeType    embedding_size  = 200;            // dimension of embedding vec
  SizeType    training_epochs = 15;             // total number of training epochs
  double      learning_rate   = 0.025;          // alpha - the learning rate
  SizeType    k               = 10;             // how many nearest neighbours to compare against
  SizeType    print_freq      = 10000;          // how often to print status
  std::string test_word       = "action";       // test word to consider
  std::string save_loc        = "./model.fba";  // save file location for exporting graph
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

  ret.window_size         = SizeType(5);   // max size of context window one way
  ret.min_sentence_length = SizeType(4);   //
  ret.k_negative_samples  = SizeType(25);  // number of negative examples to sample

  return ret;
}

/**
 * Read a single text file
 * @param path
 * @return
 */
std::string ReadFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
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

void PrintKNN(SkipGramLoader<ArrayType> const &dl, ArrayType const &embeddings,
              std::string const &word, SizeType k)
{
  ArrayType arr = embeddings;

  if (dl.VocabLookup(word) > dl.VocabSize())
  {
    std::cout << "WARNING! could not find [" + word + "] in vocabulary" << std::endl;
  }
  else
  {
    ArrayType one_vector = embeddings.Slice(dl.VocabLookup(word)).Copy();
    std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> output =
        fetch::math::clustering::KNNCosine(arr, one_vector, k);

    for (std::size_t j = 0; j < output.size(); ++j)
    {
      std::cout << "output.at(j).first: " << dl.VocabLookup(output.at(j).first) << std::endl;
      std::cout << "output.at(j).second: " << output.at(j).second << "\n" << std::endl;
    }
  }
}

void TestEmbeddings(Graph<ArrayType> const &g, std::string const &skip_gram_name,
                    SkipGramLoader<ArrayType> const &dl, std::string test_word, SizeType K)
{

  // first get hold of the skipgram layer by searching the return name in the graph
  std::shared_ptr<fetch::ml::layers::SkipGram<ArrayType>> sg_layer =
      std::dynamic_pointer_cast<fetch::ml::layers::SkipGram<ArrayType>>(g.GetNode(skip_gram_name));

  // next get hold of the embeddings
  std::shared_ptr<fetch::ml::ops::Embeddings<ArrayType>> embeddings =
      sg_layer->GetEmbeddings(sg_layer);

  PrintKNN(dl, embeddings->GetWeights(), test_word, K);
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
  dataloader.AddData(ReadFile(training_text));

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
  CrossEntropy<ArrayType> criterion;

  /////////////////////////////////
  /// TRAIN THE WORD EMBEDDINGS ///
  /////////////////////////////////

  std::cout << "beginning training...: " << std::endl;

  std::pair<ArrayType, SizeType> data;
  ArrayType                      input(std::vector<typename ArrayType::SizeType>({1, 1}));
  ArrayType                      context(std::vector<typename ArrayType::SizeType>({1, 1}));
  ArrayType                      gt(std::vector<typename ArrayType::SizeType>({1, tp.output_size}));
  ArrayType                      scale_factor(std::vector<typename ArrayType::SizeType>({1, 1}));

  DataType correct_score      = 0;
  DataType sum_average_scores = 0;
  DataType sum_average_count  = 0;
  DataType loss               = 0;
  DataType batch_loss         = 0;
  DataType sub_epoch_loss     = 0;

  SizeType batch_count     = 0;
  SizeType step_count      = 0;
  SizeType last_step_count = 0;

  ArrayType results;  // store predictions

  for (SizeType i = 0; i < tp.training_epochs; ++i)
  {
    dataloader.Reset();

    sum_average_scores = 0;
    sum_average_count  = 0;

    batch_count = 0;
    step_count  = 0;

    auto t1 = std::chrono::high_resolution_clock::now();

    sub_epoch_loss = 0;
    batch_loss     = 0;

    // effectively clears any leftover gradients
    g.Step(0);

    while (!dataloader.IsDone())
    {
      gt.Fill(DataType(0));

      // get random data point
      //      data = dataloader.GetRandom();
      data = dataloader.GetNext();

      // assign input and context vectors
      input.At(0, 0)   = data.first.At(0);
      context.At(0, 0) = data.first.At(1);

      // assign label
      gt.At(0, 0) = DataType(data.second);

      g.SetInput("Input", input, false);
      g.SetInput("Context", context, false);

      // forward pass
      results = g.Evaluate(output_name);

      scale_factor.At(0, 0) =
          (gt.At(0, 0) == DataType(0)) ? DataType(sp.k_negative_samples) : DataType(1);

      if (((results.At(0, 0) >= DataType(0.5)) && (gt.At(0, 0) == DataType(1))) ||
          ((results.At(0, 0) < DataType(0.5)) && (gt.At(0, 0) == DataType(0))))
      {
        ++correct_score;
      }

      loss = criterion.Forward({results, gt});

      // diminish size of updates due to negative examples
      if (data.second == 0)
      {
        loss /= DataType(sp.k_negative_samples);
      }
      batch_loss += loss;

      // backprop
      g.BackPropagate(output_name, criterion.Backward(std::vector<ArrayType>({results, gt})));

      // take mini-batch learning step
      if (step_count % tp.batch_size == (tp.batch_size - 1))
      {
        g.Step(tp.learning_rate);

        // average prediction scores
        sum_average_scores += (correct_score / double(tp.batch_size));
        sum_average_count++;
        correct_score = 0;

        // sum epoch losses
        sub_epoch_loss += batch_loss;
        batch_loss = 0;

        ++batch_count;
      }
      ++step_count;
      ++last_step_count;

      if (step_count % tp.print_freq == (tp.print_freq - 1))
      {
        auto                          t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_diff =
            std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);

        std::cout << "loss: " << sub_epoch_loss / double(last_step_count) << std::endl;
        std::cout << "average_score: " << sum_average_scores / sum_average_count << std::endl;
        std::cout << "over [" << batch_count << "] batches involving [" << step_count
                  << "] steps total." << std::endl;
        std::cout << "words/sec: " << double(last_step_count) / time_diff.count() << std::endl;
        std::cout << "\n: " << std::endl;
        t1              = std::chrono::high_resolution_clock::now();
        last_step_count = 0;
        sub_epoch_loss  = 0;
      }
    }

    // print batch loss and embeddings distances
    // Test trained embeddings
    TestEmbeddings(g, output_name, dataloader, tp.test_word, tp.k);

    // Save model
    fetch::ml::examples::SaveModel(g, tp.save_loc);
  }

  //////////////////////////////////////
  /// EXTRACT THE TRAINED EMBEDDINGS ///
  //////////////////////////////////////

  // Test trained embeddings
  TestEmbeddings(g, output_name, dataloader, tp.test_word, tp.k);

  return 0;
}
