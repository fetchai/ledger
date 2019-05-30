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
#include "ml/ops/activations/softmax.hpp"
#include "ml/ops/concatenate.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/loss_functions/cross_entropy.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/tanh.hpp"
#include "ml/ops/transpose.hpp"
#include "ml/ops/weights.hpp"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

using DataType   = double;
using ArrayType  = fetch::math::Tensor<DataType>;
using SizeType   = fetch::math::Tensor<DataType>::SizeType;
using SizeVector = fetch::math::SizeVector;

using Weights        = fetch::ml::ops::Weights<ArrayType>;
using Embeddings     = fetch::ml::ops::Embeddings<ArrayType>;
using Transpose      = fetch::ml::ops::Transpose<ArrayType>;
using MatrixMultiply = fetch::ml::ops::MatrixMultiply<ArrayType>;

using ContextTensorTuple      = typename std::tuple<ArrayType, ArrayType, ArrayType>;
using ContextTensorsLabelPair = typename std::pair<ContextTensorTuple, SizeType>;

#define EMBEDDING_SIZE 64u
#define BATCHSIZE 12u
#define N_EPOCHS 100
#define LEARNING_RATE 0.01f

std::string ReadFile(std::string const &path)
{
  std::ifstream t(path);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

int main(int ac, char **av)
{
  if (ac < 2)
  {
    std::cerr << "Usage: " << av[0] << " INPUT_FILES_TXT" << std::endl;
    return 1;
  }

  fetch::ml::dataloaders::C2VLoader<std::tuple<ArrayType, ArrayType, ArrayType>, SizeType> cloader(
      20);

  for (int i(1); i < ac; ++i)
  {
    cloader.AddData(ReadFile(av[i]));
  }

  std::cout << "Number of different function names: " << cloader.function_name_counter().size()
            << std::endl;
  std::cout << "Number of different paths: " << cloader.path_counter().size() << std::endl;
  std::cout << "Number of different words: " << cloader.word_counter().size() << std::endl;

  uint64_t vocab_size_function_names{cloader.function_name_counter().size() + 1};
  uint64_t vocab_size_paths{cloader.path_counter().size() + 1};
  uint64_t vocab_size_words{cloader.word_counter().size() + 1};

  // Defining the graph
  fetch::ml::Graph<ArrayType> g;

  // Setting up the attention vector
  // Dimension: (EMBEDDING_SIZE, 1)
  std::string attention_vector = g.AddNode<Weights>("AttentionVector", {});
  ArrayType   attention_vector_data(SizeVector({EMBEDDING_SIZE, SizeType{1}}));
  Weights::Initialise(attention_vector_data, EMBEDDING_SIZE, SizeType{1});
  g.SetInput(attention_vector, attention_vector_data, false);

  // Setting up the weights of FC1
  // Dimension: (EMBEDDING_SIZE, 3*EMBEDDING_SIZE)
  std::string fc1_weights = g.AddNode<Weights>("FullyConnectedWeights", {});
  ArrayType   fc1_weights_data(SizeVector({EMBEDDING_SIZE, 3 * EMBEDDING_SIZE}));
  Weights::Initialise(fc1_weights_data, EMBEDDING_SIZE, 3 * EMBEDDING_SIZE);
  g.SetInput(fc1_weights, fc1_weights_data, false);

  // Setting up the embedding matrix for the function names
  // Dimension: (VOCAB_SIZE_FUNCTION_NAMES, EMBEDDING_SIZE)
  std::string function_name_embedding = g.AddNode<Weights>("EmbeddingFunctionNames", {});
  ArrayType function_name_embedding_matrix(SizeVector({vocab_size_function_names, EMBEDDING_SIZE}));
  Weights::Initialise(function_name_embedding_matrix, vocab_size_function_names, EMBEDDING_SIZE);
  g.SetInput(function_name_embedding, function_name_embedding_matrix, false);

  // Setting up shared embedding matrix for words
  // Dimension: (VOCAB_SIZE_WORDS, EMBEDDING_SIZE)
  std::string shared_embedding = g.AddNode<Weights>("SharedEmbedding", {});
  ArrayType   shared_embedding_tensor(SizeVector({vocab_size_words, EMBEDDING_SIZE}));
  Weights::Initialise(shared_embedding_tensor, vocab_size_words, EMBEDDING_SIZE);
  g.SetInput(shared_embedding, shared_embedding_tensor, false);

  // Defining the input nodes

  // Inputs have dimensions (N_CONTEXTS, )
  std::string input_paths = g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("InputPaths", {});
  std::string input_source_words =
      g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("InputSourceWords", {});
  std::string input_target_words =
      g.AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("InputTargetWords", {});

  // Retrieving the rows of the embedding tensors according to the input

  // Path embedding
  // Dimension: (N_CONTEXTS, EMBEDDING_SIZE)
  std::string embeddings_paths =
      g.AddNode<Embeddings>("EmbeddingPaths", {input_paths}, vocab_size_paths, EMBEDDING_SIZE);

  // Target word embedding
  // Dimension: (N_CONTEXTS, EMBEDDING_SIZE)
  std::string embedding_target_words =
      g.AddNode<Embeddings>("EmbeddingTargetwords", {input_target_words}, shared_embedding_tensor);

  // Source word embedding
  // Dimension: (N_CONTEXTS, EMBEDDING_SIZE)
  std::string embedding_source_words =
      g.AddNode<Embeddings>("EmbeddingSourcewords", {input_source_words}, shared_embedding_tensor);

  // Concatenate along axis = 1
  // Dimension: (N_CONTEXTS, 3*EMBEDDING_SIZE) = Concatenate ((N_CONTEXTS, EMBEDDING_SIZE),
  // (N_CONTEXTS, EMBEDDING_SIZE), (N_CONTEXTS, EMBEDDING_SIZE))
  std::string context_vectors = g.AddNode<fetch::ml::ops::Concatenate<ArrayType>>(
      "ContextVectors", {embedding_source_words, embeddings_paths, embedding_target_words},
      SizeType(1));

  // Transposition
  // Dimensions: (3*EMBEDDING_SIZE, N_CONTEXTS) = Transpose((N_CONTEXTS, 3*EMBEDDING_SIZE))
  std::string context_vector_transpose =
      g.AddNode<Transpose>("ContextVectorTransposed", {context_vectors});

  // Fully connected layer
  // Dimensions: (EMBEDDING_SIZE, N_CONTEXTS) = (EMBEDDING_SIZE, 3*EMBEDDING_SIZE) @
  // (3*EMBEDDING_SIZE, N_CONTEXTS)
  std::string fc1 = g.AddNode<MatrixMultiply>("FC1", {fc1_weights, context_vector_transpose});

  // (Elementwise) TanH Layer
  // Dimensions: (EMBEDDING_SIZE, N_CONTEXTS)
  std::string combined_context_vector =
      g.AddNode<fetch::ml::ops::TanH<ArrayType>>("CombinedContextVector", {fc1});

  // Transposition
  // Dimensions: (N_CONTEXTS, EMBEDDING_SIZE) = Transpose((EMBEDDING_SIZE, N_CONTEXTS))
  std::string combined_context_vector_transpose =
      g.AddNode<Transpose>("CombinedContextVectorTransposed", {combined_context_vector});

  // (Dot) Multiplication with the Attention vector
  // Dimensions: (N_CONTEXTS, 1) = (N_CONTEXTS, EMBEDDING_SIZE) @ (EMBEDDING_SIZE, 1)
  std::string scalar_product_contexts_with_attention = g.AddNode<MatrixMultiply>(
      "ScalarProductContextsWithAttention", {combined_context_vector_transpose, attention_vector});

  // Transposition
  // DImension: (1, N_CONTEXTS) = Transpose((N_CONTEXTS, 1))
  std::string scalar_product_contexts_with_attention_transposed = g.AddNode<Transpose>(
      "ScalarProductContextsWithAttentionTransposed", {scalar_product_contexts_with_attention});

  // (Softmax) normalisation
  // Dimensions: (1, N_CONTEXTS)
  std::string attention_weight = g.AddNode<fetch::ml::ops::Softmax<ArrayType>>(
      "AttentionWeight", {scalar_product_contexts_with_attention_transposed});

  // Transposition
  //   Dimensions: (N_CONTEXTS, 1)
  std::string attention_weight_transposed =
      g.AddNode<Transpose>("AttentionWeightTransposed", {attention_weight});

  // (Dot) Multiplication with attention weights; i.e. calculating the code vectors
  // Dimensions: (EMBEDDING_SIZE, 1) = (EMBEDDING_SIZE, N_CONTEXTS) @ (N_CONTEXTS, 1)
  std::string code_vector = g.AddNode<MatrixMultiply>(
      "CodeVector", {combined_context_vector, attention_weight_transposed});

  // (Unnormalised) predictions for each function name in the vocab, by
  // matrix multiplication with the embedding tensor
  // Dimensions: (vocab_size_functions, 1) = (vocab_size_functions, EMBEDDING_SIZE) @
  // (EMBEDDING_SIZE, 1)
  std::string prediction_softmax_kernel =
      g.AddNode<MatrixMultiply>("PredictionSoftMaxKernel", {function_name_embedding, code_vector});

  // Dimensions: (1, vocab_size_functions) = Transpose((vocab_size_functions, 1))
  std::string prediction_softmax_kernel_T =
      g.AddNode<Transpose>("PredictionSoftMaxKernelTransposed", {prediction_softmax_kernel});

  // (Softmax) Normalisation of the prediction
  // Dimensions:  (1, vocab_size_functions)
  std::string result = g.AddNode<fetch::ml::ops::Softmax<ArrayType>>(
      "PredictionSoftMax", {prediction_softmax_kernel_T}, SizeType{0});

  // Criterion: Cross Entropy Loss
  // Here, the CrossEntropy eats two tensors of size (1, function_name_vocab_size); i.e. it has 1
  // example, and as many categories as vocab_size
  fetch::ml::ops::CrossEntropy<ArrayType> criterion;
  DataType                                loss = 0;

  // (One hot encoded) \y_{true} vector
  ArrayType y_true_vec({1, cloader.function_name_counter().size() + 1});

  y_true_vec.Fill(0);

  int n_epochs{0};
  int n_iter{0};

  while (n_epochs < N_EPOCHS)
  {
    if (cloader.IsDone())
    {
      cloader.Reset();
      n_epochs++;
    }

    auto t1 = std::chrono::high_resolution_clock::now();

    // Loading the tuple of ((InputSourceWords, InputPaths, InputTargetWords), function_name_idx)
    // first: 3 tensors with shape (n_contexts) holding
    //        the indices of the source words/paths/target words in the vocabulary
    // second: function_name_idx is the index of the function name in the vocabulary
    ContextTensorsLabelPair input = cloader.GetNext();

    // Feeding the tensors to the graph
    g.SetInput(input_source_words, std::get<0>(input.first));
    g.SetInput(input_paths, std::get<1>(input.first));
    g.SetInput(input_target_words, std::get<2>(input.first));

    // Preparing the y_true vector (one-hot-encoded)
    y_true_vec.Set(0, input.second, 1);

    // Making the forward pass
    // dimension:  (1, vocab_size_functions)
    ArrayType results = g.Evaluate(result);
    // dimension:  (1, vocab_size_functions), (1, vocab_size_functions)
    loss += criterion.Forward({results, y_true_vec});

    // Making the backward pass
    g.BackPropagate(result, criterion.Backward({results, y_true_vec}));

    // apply the gradients
    g.Step(LEARNING_RATE);

    // Resetting the y_true vector for reusing it
    y_true_vec.Set(0, input.second, 0);

    n_iter++;
    if (n_iter % 5 == 0)
    {
      auto t2 = std::chrono::high_resolution_clock::now();

      std::chrono::duration<double> time_diff =
          std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);

      std::cout << "MiniBatch: " << n_iter / 5 << " -- Loss : " << loss << std::endl;
      std::cout << "Steps / Sec: " << 5.0 / time_diff.count() << std::endl;
      loss = 0;
    }
  }

  return 0;
}
