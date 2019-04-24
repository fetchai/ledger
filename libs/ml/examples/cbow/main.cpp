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

#include "math/tensor.hpp"

#include "ml/dataloaders/w2v_cbow_dataloader.hpp"
#include "ml/graph.hpp"
#include "ml/misc/unigram_table.hpp"
#include "ml/ops/activations/sigmoid.hpp"
#include "ml/ops/averaged_embeddings.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/inplace_transpose.hpp"
#include "ml/ops/loss_functions/mean_square_error.hpp"
#include "ml/ops/matrix_multiply.hpp"

#include <vector>

#define WINDOW_SIZE 5u
#define MIN_WORD_FREQUENCY 5u
#define EMBEDDING_DIMENSION 100u
#define EPOCHS 1u
#define NEGATIVE_SAMPLES 25u

using DataType  = float;
using ArrayType = fetch::math::Tensor<DataType>;

std::string ReadFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

int main(int ac, char **av)
{
  std::cout << "Word2Vec Demo" << std::endl;
  if (ac < 2)
  {
    std::cout << "Usage : " << av[0] << " TRAINING_CORPUS_FILES ..." << std::endl;
    return 0;
  }

  fetch::ml::CBOWLoader<DataType> loader(WINDOW_SIZE);
  for (int i(1); i < ac; ++i)
  {
    loader.AddData(ReadFile(av[i]));
  }
  loader.RemoveInfrequent(MIN_WORD_FREQUENCY);
  std::vector<uint64_t> frequencies(loader.VocabSize());
  for (auto const &kvp : loader.GetVocab())
  {
    frequencies[kvp.second.first] = kvp.second.second;
  }
  UnigramTable unigram_table(1e8, frequencies);

  auto sample = loader.GetNext();

  fetch::ml::Graph<ArrayType> g;
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input_Context",
                                                    {});  // A list of IDs of the context words
  g.AddNode<fetch::ml::ops::AveragedEmbeddings<ArrayType>>(
      "Embeddings", {"Input_Context"}, loader.VocabSize(),
      EMBEDDING_DIMENSION);  // This embedding matrix will contain the word vectors
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input_Target",
                                                    {});  // The ID of the target word
  g.AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
      "Weights", {"Input_Target"}, loader.VocabSize(),
      EMBEDDING_DIMENSION);  // This embedding matrix will contain weights
  g.AddNode<fetch::ml::ops::InplaceTranspose<ArrayType>>("Transpose", {"Weights"});
  g.SetInput("Input_Target", sample.second);  // TODO(private, 890)

  g.AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>(
      "Dot", {"Embeddings", "Transpose"});  // Dot Product Vector x Weights
  g.AddNode<fetch::ml::ops::Sigmoid<ArrayType>>("Sigmoid", {"Dot"});  // Activation
  fetch::ml::ops::MeanSquareError<ArrayType> mse;

  // Print first sample for demonstration purposes / sanity check

  for (unsigned int i(0); i < sample.first.size(); ++i)
  {
    if (i == sample.first.size() / 2)
    {
      std::cout << "[" << loader.WordFromIndex((uint64_t)sample.second.At(0)) << "] ";
    }
    std::cout << loader.WordFromIndex((uint64_t)sample.first.At(i)) << " ";
  }
  std::cout << std::endl;

  float     alpha = 0.05f;
  ArrayType label({1, 1});
  for (unsigned int epoch(0); epoch < EPOCHS; ++epoch)
  {
    std::cout << "Epoch " << epoch << std::endl;
    loader.Reset();
    float        loss = 0;
    unsigned int i(0);
    while (!loader.IsDone())
    {
      i++;
      if (i % 1000 == 0)
      {
        std::cout << i << " / " << loader.Size() << std::endl;
      }
      sample = loader.GetNext(sample);
      g.SetInput("Input_Context", sample.first);
      for (unsigned int n(0); n < NEGATIVE_SAMPLES; ++n)
      {
        if (n == 0)
        {
          label.At(0, 0) = 1.0f;
        }
        else
        {
          label.At(0, 0)           = 0.0f;
          DataType negative_sample = DataType(unigram_table.Sample());
          while (negative_sample == sample.second.At(0))
          {
            negative_sample = DataType(unigram_table.Sample());
          }
          sample.second.At(0) = negative_sample;
        }
        g.SetInput("Input_Target", sample.second);
        ArrayType pred = g.Evaluate("Sigmoid");
        loss += mse.Forward({pred, label});
        g.BackPropagate("Sigmoid", mse.Backward({pred, label}));
      }
      g.Step(alpha);
    }
    std::cout << "Loss : " << loss << std::endl;
  }
  return 0;
}
