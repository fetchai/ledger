
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

#include "ml/graph.hpp"
#include "math/tensor.hpp"
//#include "ml/dataloaders/code2vec_context_loaders/context_loader.hpp"
#include "ml/ops/activations/softmax.hpp"

#include "ml/layers/fully_connected.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/activation_functions/softmax.hpp"
#include "ml/ops/loss_functions/cross_entropy.hpp"
#include "ml/ops/tanh.hpp"
#include "ml/ops/transpose.hpp"
#include <fstream>
#include <iostream>

using DataType  = int;
using ArrayType = fetch::math::Tensor<DataType>;
using SizeType  = fetch::math::Tensor<DataType>::SizeType;

using ContextTensorTuple      = typename std::tuple<ArrayType, ArrayType, ArrayType>;
using ContextTensorsLabelPair = typename std::pair<ContextTensorTuple, SizeType>;


#define EMBEDDING_SIZE 64u
#define BATCHSIZE 12u
#define N_EPOCHS 3

std::string readFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

int main(int ac, char** av)
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

  ArrayType AttentionVector({EMBEDDING_SIZE, SizeType{1}});

  //Defining the graph
  fetch::ml::Graph<ArrayType> g;
  
    

  //Defining the input nodes

  // Inputs have dimensions (N_CONTEXTS, )
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("InputPaths", {});
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("InputSourceWords", {});
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("InputTargetWords", {});

  // Dimension ()
  g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("InputFunctionNames", {});

  // Retrieving the rows of the embedding tensors according to the input
  // Path embedding
  g.AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
      "EmbeddingPaths", {"InputPaths"}, cloader.GetCounterPaths().size(), EMBEDDING_SIZE);
  // Target word embedding
  g.AddNode<fetch::ml::ops::Embeddings<ArrayType>>("EmbeddingTargetwords", {"InputTargetWords"},
                                                   cloader.GetCounterWords().size(),
                                                   EMBEDDING_SIZE);
  // Source word embedding, sharing the embedding tensor with the target word (c.f. paper and tf implementation)
  g.AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
      "EmbeddingSourcewords", {"InputSourceWords"},
      std::dynamic_pointer_cast<fetch::ml::ops::Embeddings<ArrayType>>(
          g.GetNode("EmbeddingTargetwords"))
          ->GetWeights());

  g.AddNode<fetch::ml::ops::Embeddings<ArrayType>>("EmbeddingFunctionnames", {"InputFunctionNames"},
                                                   cloader.GetCounterFunctionNames().size(),
                                                   EMBEDDING_SIZE);

  // Concatenate along axis = 1
  // Dimension: (N_CONTEXTS, 3*EMBEDDING_SIZE) = Concatenate ((N_CONTEXTS, EMBEDDING_SIZE), (N_CONTEXTS, EMBEDDING_SIZE), (N_CONTEXTS, EMBEDDING_SIZE))
  g.AddNode<Concatenate<ArrayType>>("ContextVectors", {"EmbeddingSourcewords",
  "EmbeddingPaths", "EmbeddingTargetwords"}, 1);

  // Fully connected layer
  // REMARK: In the original implementation its without bias
  // Dimensions: (N_CONTEXTS, EMBEDDING_SIZE) = (EMBEDDING_SIZE, 3*EMBEDDING_SIZE) @ (N_CONTEXTS, 3*EMBEDDING_SIZE)
  g.AddNode<fetch::ml::layers::FullyConnected<ArrayType>>("FC1", {"ContextVectors", "FC"},
  3*EMBEDDING_SIZE, EMBEDDING_SIZE);
  
  // (Elementwise) TanH Layer
  // Dimensions: (N_CONTEXTS, EMBEDDING_SIZE) 
  g.AddNode<fetch::ml::ops::TanH<ArrayType>>("CombinedContextVector", {"FC1"});
  // Dimensions: (EMBEDDING_SIZE, N_CONTEXTS) = Transposed (N_CONTEXTS, EMBEDDING_SIZE)
  g.AddNode<fetch::ml::ops::Transpose<ArrayType>>("CombinedContextVectorTransposed", {"CombinedContextVector"});

  // (Dot) Multiplication with the Attention vector
  // Dimensions: (N_CONTEXTS, 1) = (N_CONTEXTS, EMBEDDING_SIZE) @ (EMBEDDING_SIZE, 1)
  g.AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>("ScalarProductContextsWithAttention", {"CombinedContextVector",
  "AttentionVector"});

  // (Softmax) normalisation along axis 0
  // Dimensions: (N_CONTEXTS, 1)
  g.AddNode<fetch::ml::ops::Softmax<ArrayType>>("AttentionWeight", {"ScalarProductContextsWithAttention"});

  // (Dot) Multiplication with attention weights; i.e. calculating the code vectors
  // Dimensions: (EMBEDDING_SIZE, 1) = (EMBEDDING_SIZE, N_CONTEXTS) @ (N_CONTEXTS, 1) 
  g.AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>("CodeVector", {"CombinedContextVectorTransposed", "AttentionWeight"});

  // (Unnormalised) predictions for each function name in the vocab, by
  // matrix multiplication with the embedding tensor
  // Dimensions: (vocab_size_functions, 1) = (vocab_size_functions, EMBEDDING_SIZE) @ (EMBEDDING_SIZE, 1)
  g.AddNode<fetch::ml::ops::MatrixMultiply<ArrayType>>("PredictionSoftMaxKernel", {std::dynamic_pointer_cast<fetch::ml::ops::Embeddings<ArrayType>>(
          g.GetNode("InputFunctionNames"))
          ->GetWeights(),"CodeVector"})

  // (Softmax) Normalisation of the prediction
  // Dimensions: (vocab_size_functions, 1)
  g.AddNode<fetch::ml::ops::Softmax<ArrayType>>("PredictionSoftMax", {"PredictionSoftMaxKernel"});
  // Dimensions: (1, vocab_size_functions) = Transpose (vocab_size_functions, 1)
  std::string result = g.AddNode<fetch::ml::ops::Transpose<ArrayType>>("PredictionSoftMaxTransposed", {"PredictionSoftMax"});

  // Criterion: Cross Entropy Loss
  // Here, the CrossEntropy eats two tensors of size (1, function_name_vocab_size); i.e. it has 1 example, and as many categories as vocab_size
  fetch::ml::ops::CrossEntropy<ArrayType> criterion;
  DataType     loss = 0;

  // (One hot encoded) \y_{true} vector
  fetch::math::Tensor<DataType> y_true_vec({1, cloader.GetCounterFunctionNames().size()});

  for(u_int64_t i{0}; i < y_true_vec.shape()[1]; i++){
    y_true_vec.Set(0, i, 0);
  }

  int n_epochs{0};
  while (n_epochs < N_EPOCHS)
  {
    if (cloader.IsDone())
    {
      cloader.Reset();
      n_epochs++;
    }

    // Loading the tuple of ((InputSourceWords, InputPaths, InputTargetWords), function_name_idx)
    // first: 3 tensors with shape (n_contexts) holding 
    //        the indices of the source words/paths/target words in the vocabulary
    // second: function_name_idx is the index of the function name in the vocabulary
    ContextTensorsLabelPair input = cloader.GetNext();

    // Feeding the tensors to the graph
    g.SetInput("InputSourceWords", std::get<0>(input.first));
    g.SetInput("InputPaths", std::get<1>(input.first));
    g.SetInput("InputTargetWords", std::get<2>(input.first));

    // Preparing the y_true vector (one-hot-encoded)
    SizeType y_true_i{input.second};
    y_true_vec.Set(0, y_true_i, 1);

    // Making the forward pass
    // dimension:  (1, vocab_size_functions)
    ArrayType results = g.Evaluate(result);
    // dimension:  (1, vocab_size_functions), (1, vocab_size_functions)
    loss = criterion.Forward({results, y_true_vec});

    // Making the backward pass
    g.BackPropagate(result, criterion.Backward({results, y_true_vec}));

    // Resetting the y_true vector for reusing it
    y_true_vec.Set(0, y_true_i, 1);
  }

  return 0;
}
