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

#include "ml/dataloaders/w2v_cbow_dataloader.hpp"
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

using DataType  = float;
using ArrayType = fetch::math::Tensor<DataType>;

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

std::string readFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

std::string findWordByIndex(std::map<std::string, uint64_t> const &vocab, uint64_t index)
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

void PrintKNN(std::map<std::string, uint64_t> const &vocab, ArrayType const &embeddings,
              std::string const &word, unsigned int k)
{
  ArrayType                               wordVector = embeddings.Slice(vocab.at(word)).Unsqueeze();
  std::vector<std::pair<uint64_t, float>> distances;
  distances.reserve(embeddings.shape()[0]);
  for (uint64_t i(1); i < embeddings.shape()[0]; ++i)  // Start at 1, 0 is UNKNOWN
  {
    DataType d = fetch::math::distance::Cosine(wordVector, embeddings.Slice(i).Unsqueeze());
    distances.emplace_back(i, d);
  }
  std::nth_element(distances.begin(), distances.begin() + k, distances.end(),
                   [](std::pair<uint64_t, float> const &a, std::pair<uint64_t, float> const &b) {
                     return a.second < b.second;
                   });
  std::cout << "======================" << std::endl;
  for (uint64_t i(0); i < k; ++i)
  {
    std::cout << findWordByIndex(vocab, distances[i].first) << " -- " << distances[i].second
              << std::endl;
  }
}

int main(int ac, char **av)
{
  if (ac < 2)
  {
    std::cerr << "Usage: " << av[0] << " INPUT_FILES_TXT" << std::endl;
    return 1;
  }

  fetch::ml::CBOWLoader<DataType> loader(CONTEXT_WINDOW_SIZE);
  for (int i(1); i < ac; ++i)
  {
    loader.AddData(readFile(av[i]));
  }

  unsigned int vocabSize = (unsigned int)loader.GetVocab().size();
  std::cout << "Vocab size : " << vocabSize << std::endl;

  fetch::ml::Graph<ArrayType> g;
  g.AddNode<PlaceHolder<ArrayType>>("Input", {});
  g.AddNode<Embeddings<ArrayType>>("Embeddings", {"Input"}, vocabSize, EMBEDING_DIMENSION);
  g.AddNode<FullyConnected<ArrayType>>("FC", {"Embeddings"},
                                       EMBEDING_DIMENSION * CONTEXT_WINDOW_SIZE * 2, vocabSize);
  g.AddNode<Softmax<ArrayType>>("Softmax", {"FC"});

  MeanSquareError<ArrayType> criterion;
  unsigned int               iteration(0);
  float                      loss = 0;

  unsigned int epoch(0);
  while (true)
  {
    loader.Reset();
    while (!loader.IsDone())
    {
      auto input = loader.GetNext();
      g.SetInput("Input", input.first);
      ArrayType predictions = g.Evaluate("Softmax");
      ArrayType groundTruth(predictions.shape());
      groundTruth.At(input.second) = DataType(1);

      uint64_t argmax(uint64_t(ArgMax(predictions)));
      if (iteration % 100 == 0 || argmax == input.second)
      {
        for (unsigned int i(0); i < CONTEXT_WINDOW_SIZE * 2 + 1; ++i)
        {
          if (i < CONTEXT_WINDOW_SIZE)
          {
            std::cout << findWordByIndex(loader.GetVocab(), uint64_t(input.first.At(i))) << " ";
          }
          else if (i == CONTEXT_WINDOW_SIZE)
          {
            std::cout << "[" << findWordByIndex(loader.GetVocab(), uint64_t(input.second)) << "] ";
          }
          else
          {
            std::cout << findWordByIndex(loader.GetVocab(), uint64_t(input.first.At(i - 1))) << " ";
          }
        }
        std::cout << "-- " << (argmax == input.second ? "\033[0;32m" : "\033[0;31m")
                  << findWordByIndex(loader.GetVocab(), argmax) << "\033[0;0m" << std::endl;
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
    PrintKNN(loader.GetVocab(), *g.StateDict().dict_["Embeddings"].weights_, "one", 6);

    // Save model
    fetch::serializers::ByteArrayBuffer serializer;
    serializer << g.StateDict();
    std::fstream file("./model.fba");  // fba = FetchByteArray
    file << std::string(serializer.data());
    file.close();
    epoch++;
  }
  return 0;
}
