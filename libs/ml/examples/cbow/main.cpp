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

#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include "math/tensor.hpp"

#include "ml/dataloaders/w2v_cbow_dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activations/softmax.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/loss_functions/mean_square_error.hpp"

#include <clocale>
#include <iostream>

#define EMBEDING_DIMENSION 64u
#define CONTEXT_WINDOW_SIZE 4  // Each side
#define LEARNING_RATE 0.01f

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

int main(int ac, char **av)
{
  fetch::ml::CBOWLoader<DataType> loader(CONTEXT_WINDOW_SIZE);
  loader.AddData(readFile("/Users/wilmot_p/gutenberg/4634-0.txt"));
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
  while (!loader.IsDone())
  {
    auto input = loader.GetNext();
    g.SetInput("Input", input.first);
    ArrayType predictions = g.Evaluate("Softmax");
    ArrayType groundTruth(predictions.shape());
    groundTruth.At(input.second) = DataType(1);
    loss += criterion.Forward({predictions, groundTruth});
    g.BackPropagate("Softmax", criterion.Backward({predictions, groundTruth}));
    g.Step(LEARNING_RATE);

    if (iteration % 1 ==
        0)  // At the moment, 1 iteration runs in 7 sec >< Time to do some optimisation
    {
      for (unsigned int i(0); i < CONTEXT_WINDOW_SIZE * 2 + 1; ++i)
      {
        if (i < CONTEXT_WINDOW_SIZE)
          std::cout << findWordByIndex(loader.GetVocab(), uint64_t(input.first.At(i))) << " ";
        else if (i == CONTEXT_WINDOW_SIZE)
          std::cout << "[" << findWordByIndex(loader.GetVocab(), uint64_t(input.second)) << "] ";
        else
          std::cout << findWordByIndex(loader.GetVocab(), uint64_t(input.first.At(i - 1))) << " ";
      }
      std::cout << "-- " << findWordByIndex(loader.GetVocab(), uint64_t(ArgMax(predictions)))
                << std::endl;
      std::cout << "Loss : " << loss << std::endl;
      loss = 0;
    }
    iteration++;
  }
  return 0;
}
