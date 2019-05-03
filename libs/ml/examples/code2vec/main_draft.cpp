
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
#include "ml/dataloaders/code2vec_context_loaders/context_loader.hpp"
#include "ml/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/tanh.hpp"
#include <fstream>
#include <iostream>

using DataType  = int;
using ArrayType = fetch::math::Tensor<DataType>;
using SizeType  = fetch::math::Tensor<DataType>::SizeType;

#define EMBEDDING_SIZE 64u
#define BATCHSIZE 12u
#define N_EPOCHS 3

std::string readFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

int main()
{
  if (ac < 2)
  {
    std::cerr << "Usage: " << av[0] << " INPUT_FILES_TXT" << std::endl;
    return 1;
  }

  fetch::ml::dataloaders::C2VLoader<std::tuple<ArrayType, ArrayType, ArrayType>, SizeType> cloader;

  for (int i(1); i < ac; ++i)
  {
    cloader.AddData(readFile(av[i]));
  }

  std::cout << "Number of different function names: " << cloader.GetCounterFunctionNames().size()
            << std::endl;
  std::cout << "Number of different paths: " << cloader.GetCounterPaths().size() << std::endl;
  std::cout << "Number of different words: " << cloader.GetCounterWords().size() << std::endl;

  fetch::ml::Graph<ArrayType> g;
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("InputPaths", {});
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("InputSourceWords", {});
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("InputTargetWords", {});
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("InputFunctionNames", {});

  g.AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
      "EmbeddingPaths", {"InputPaths"}, cloader.GetCounterPaths().size(), EMBEDDING_SIZE);
  g.AddNode<fetch::ml::ops::Embeddings<ArrayType>>("EmbeddingTargetwords", {"InputTargetWords"},
                                                   cloader.GetCounterWords().size(),
                                                   EMBEDDING_SIZE);
  g.AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
      "EmbeddingSourcewords", {"InputSourceWords"},
      std::dynamic_pointer_cast<fetch::ml::ops::Embeddings<ArrayType>>(
          g.GetNode("EmbeddingTargetwords"))
          ->GetWeights());
  g.AddNode<fetch::ml::ops::Embeddings<ArrayType>>("EmbeddingFunctionnames", {"InputFunctionNames"},
                                                   cloader.GetCounterFunctionNames().size(),
                                                   EMBEDDING_SIZE);

  // Concatenate along axis = 1
  // g.AddNode<Concatenate<ArrayType>>("ContextVectors", {"EmbeddingSourcewords",
  //"EmbeddingPaths", "EmbeddingTargetwords"}, 1);

  // In the original implementation its without bias
  // g.AddNode<fetch::ml::layers::FullyConnected<ArrayType>>("FC1", {"ContextVectors", "FC"},
  // BATCHSIZE, 3*EMBEDDING_SIZE); g.AddNode<fetch::ml::ops::TanH<ArrayType>>("TanH", {"FC1"});
  // g.AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>("ContextWeights", {"TanH",
  //"AttentionParameters"});

  int n_epochs{0};
  while (n_epochs < N_EPOCHS)
  {
    if (cloader.IsDone())
    {
      cloader.Reset();
      n_epochs++;
    }

    auto input = cloader.GetNext();

    g.SetInput("InputSourceWords", std::get<0>(input.first));
    g.SetInput("InputPaths", std::get<1>(input.first));
    g.SetInput("InputTargetWords", std::get<2>(input.first));
  }

  return 0;
}
