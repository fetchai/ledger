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
#include "ml/dataloaders/word2vec_loaders/skipgram_dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/skip_gram.hpp"
#include "ml/ops/loss_functions/cross_entropy.hpp"
#include "ml/optimisation/adam_optimiser.hpp"

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

////////////////////////////////
/// PARAMETERS AND CONSTANTS ///
////////////////////////////////

struct TrainingParams
{
  SizeType    output_size     = 1;
  SizeType    batch_size      = 128;            // training data batch size
  SizeType    embedding_size  = 32;             // dimension of embedding vec
  SizeType    training_epochs = 100000;         // total number of training epochs
  double      learning_rate   = 0.1;            // alpha - the learning rate
  SizeType    k               = 10;             // how many nearest neighbours to compare against
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
  ret.k_negative_samples  = SizeType(10);  // number of negative examples to sample

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

  PrintKNN(dl, embeddings->get_weights(), test_word, K);
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
  SkipGramLoader<ArrayType> data_loader(sp, true);

  // load text from files as necessary and process text with dataloader
  data_loader.AddData(fetch::ml::examples::GetTextString(training_text));

  std::cout << "dataloader.VocabSize(): " << data_loader.VocabSize() << std::endl;
  std::cout << "dataloader.Size(): " << data_loader.Size() << std::endl;

  ////////////////////////////////
  /// SETUP MODEL ARCHITECTURE ///
  ////////////////////////////////

  // set up model architecture
  std::cout << "building model architecture...: " << std::endl;
  std::shared_ptr<fetch::ml::Graph<ArrayType>> g(std::make_shared<fetch::ml::Graph<ArrayType>>());
  std::string output_name = Model(*g, tp.embedding_size, data_loader.VocabSize());

  // set up loss
  CrossEntropy<ArrayType> criterion;

  /////////////////////////////////
  /// TRAIN THE WORD EMBEDDINGS ///
  /////////////////////////////////

  std::cout << "beginning training...: " << std::endl;

  // Initialise Optimiser
  fetch::ml::optimisers::AdamOptimiser<ArrayType, fetch::ml::ops::CrossEntropy<ArrayType>>
      optimiser(g, {"Input", "Context"}, "", output_name, tp.learning_rate);

  // Training loop
  DataType loss;
  for (SizeType i{0}; i < tp.training_epochs; i++)
  {
    loss = optimiser.Run(data_loader, tp.batch_size, tp.batch_size);
    std::cout << "Loss: " << loss << std::endl;
  }

  //////////////////////////////////////
  /// EXTRACT THE TRAINED EMBEDDINGS ///
  //////////////////////////////////////

  // Test trained embeddings
  TestEmbeddings(*g, output_name, data_loader, tp.test_word, tp.k);

  return 0;
}
