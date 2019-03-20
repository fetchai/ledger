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

#include "core/serializers/byte_array_buffer.hpp"

#include "math/distance/cosine.hpp"
#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include "math/tensor.hpp"

#include "ml/dataloaders/cbow_dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activations/softmax.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/loss_functions/mean_square_error.hpp"
#include "ml/serializers/ml_types.hpp"

#include <iostream>

#define EMBEDING_DIMENSION 64u
#define CONTEXT_WINDOW_SIZE 4  // Each side
#define LEARNING_RATE 0.50f

using DataType  = double;
using ArrayType = fetch::math::Tensor<DataType>;
using SizeType  = fetch::math::Tensor<DataType>::SizeType;

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

std::string readFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

std::string findWordByIndex(std::map<std::string, SizeType> const &vocab, SizeType index)
{
  for (auto const &kvp : vocab)
  {
    if (kvp.second == index)
    {
      return kvp.first;
    }
  }
  return "";
}

void PrintKNN(fetch::ml::dataloaders::CBoWLoader<ArrayType> &loader, ArrayType const &embeddings,
              std::string const &word, unsigned int k)
{
  std::vector<std::pair<std::string, double>> results = loader.GetKNN(embeddings, word, k);

  std::cout << "======================" << std::endl;
  for (SizeType i(0); i < k; ++i)
  {
    std::cout << results.at(i).first << " -- " << results.at(i).second << std::endl;
  }
}

int main(int ac, char **av)
{
  if (ac < 2)
  {
    std::cerr << "Usage: " << av[0] << " INPUT_FILES_TXT" << std::endl;
    return 1;
  }

  fetch::ml::dataloaders::TextParams<ArrayType> p;
  p.window_size    = CONTEXT_WINDOW_SIZE;
  p.n_data_buffers = CONTEXT_WINDOW_SIZE * 2;
  p.min_sentence_length =
      SizeType((CONTEXT_WINDOW_SIZE * 2) + 1);  // maximum number of sentences to use
  p.max_sentences = SizeType(10000);            // maximum number of sentences to use
  //  p.discard_frequent  = true;    // discard most frqeuent words
  //  p.discard_threshold = 0.0001;  // controls how aggressively to discard frequent words

  fetch::ml::dataloaders::CBoWLoader<ArrayType> loader(p);
  for (int i(1); i < ac; ++i)
  {
    loader.AddData(readFile(av[i]));
  }

  unsigned int vocabSize = (unsigned int)loader.VocabSize();
  std::cout << "Vocab size : " << vocabSize << std::endl;

  fetch::ml::Graph<ArrayType> g;
  g.AddNode<PlaceHolder<ArrayType>>("Input", {});
  g.AddNode<Embeddings<ArrayType>>("Embeddings", {"Input"}, vocabSize, EMBEDING_DIMENSION);
  g.AddNode<FullyConnected<ArrayType>>("FC", {"Embeddings"},
                                       EMBEDING_DIMENSION * CONTEXT_WINDOW_SIZE * 2, vocabSize);
  g.AddNode<Softmax<ArrayType>>("Softmax", {"FC"});

  MeanSquareError<ArrayType> criterion;
  unsigned int               iteration(0);
  double                     loss = 0;

  unsigned int epoch(0);
  while (true)
  {
    //    loader.Reset();
    while (!loader.IsDone())
    {
      auto data = loader.GetNext();
      g.SetInput("Input", data.first);
      ArrayType predictions = g.Evaluate("Softmax");
      ArrayType groundTruth(predictions.shape());
      groundTruth.At(data.second) = DataType(1);

      SizeType argmax(SizeType(ArgMax(predictions)));
      if (iteration % 100 == 0 || argmax == data.second)
      {
        for (unsigned int i(0); i < p.n_data_buffers + 1; ++i)
        {
          if (i < p.window_size)
          {
            std::cout << loader.VocabLookup(SizeType(data.first.At(i))) << " ";
          }
          else if (i == p.window_size)
          {
            std::cout << "[" << loader.VocabLookup(SizeType(data.first.At(p.window_size))) << "] ";
          }
          else
          {
            std::cout << loader.VocabLookup(SizeType(data.first.At(i - 1))) << " ";
          }
        }
        std::cout << "-- " << (argmax == data.second ? "\033[0;32m" : "\033[0;31m")
                  << loader.VocabLookup(argmax) << "\033[0;0m" << std::endl;
        std::cout << "Loss : " << loss << std::endl;
        loss = 0;
      }

      loss += criterion.Forward({predictions, groundTruth});
      g.BackPropagate("Softmax", criterion.Backward({predictions.Clone(), groundTruth}));
      g.Step(LEARNING_RATE);

      iteration++;
    }
    std::cout << "End of epoch " << epoch << std::endl;

    // Print KNN of word "one"
    PrintKNN(loader, *g.StateDict().dict_["Embeddings"].weights_, "one", 6);

    // Save model
    fetch::serializers::ByteArrayBuffer serializer;
    serializer << g.StateDict();
    std::fstream file("./model.fba", std::fstream::out);  // fba = FetchByteArray
    if (file)
    {
      file << std::string(serializer.data());
      file.close();
    }
    else
    {
      std::cerr << "Can't open save file" << std::endl;
    }
    epoch++;
  }
  return 0;
}
