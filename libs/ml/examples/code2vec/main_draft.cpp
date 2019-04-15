
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
#include "ml/graph.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/tanh.hpp"
#include <iostream>

using DataType = float;
using Tensor   = fetch::math::Tensor<DataType>;

int main()
{
  int embedding_size;

  fetch::ml::Graph<Tensor> g;

  // Define Graph

  // Create trainable tensors

  // For embedding the nodes in the graph and the paths (with shape (path_vocab + 1or words_vocab +
  // 1, embedding_size)
  Tensor SourceWordEmbedding(...);
  Tensor PathEmbedding(...);
  Tensor TargetWordEmbedding(...);

  // For the target (shape (targetinput_vocab_length + 1, 3*embedding_size))
  Tensor TargetInputEmbedding(...);

  // For the attention parameters (shape (3*embedding_size))
  Tensor AttentionParameters(...);

  // For FC without bias (shape (3*embedding_size, 3*embedding_size))
  Tensor FC(...);

  // Input (of size (batch_size))
  g.AddNode<PlaceHolder<Tensor>>("SourceWordInput", {});
  g.AddNode<PlaceHolder<Tensor>>("PathInput", {});
  g.AddNode<PlaceHolder<Tensor>>("TargetWordInput", {});
  g.AddNode<PlaceHolder<Tensor>>("TargetInput", {});

  // Make embedding (creating matrices of dim (vocab_size, embedding_size)) and retrieve rows
  // according to the inputs (output has then shape (vocab_size + 1, embedding_size)) In the
  // Background, the embedding matrix of SourceWordEmbedding and TargetWordEmbedding should be
  // shared!
  g.AddNode<RetrieveRows<Tensor>>("SourceWordEmbedding",
                                  {"SourceWordInput", "SourceWordEmbedding"});
  g.AddNode<RetrieveRows<Tensor>>("PathEmbedding", {"PathInput", "PathEmbedding"});
  g.AddNode<RetrieveRows<Tensor>>("TargetWordEmbedding",
                                  {"TargetWordInput", "TargetWordEmbedding"});

  // Embedding of the code target of shape (vocab_size, 3*embedding_size)
  g.AddNode<RetrieveRows<Tensor>>("TargetWordEmbedding", {"TargetInput", "TargetInputEmbedding"});

  // Concatenate the matrices, s.t. we have an object of dim (batch_size, 3*embedding_size)
  g.AddNode<Concatenate<Tensor>>("ContextVectors",
                                 {"SourceWordEmbedding", "PathEmbedding", "TargetWordEmbedding"});

  // Add FullyConnected of shape (3*embedding_size, 3*embedding_size) without bias vector; output
  // has shape of (batch_size, 3*embedding_size)
  g.AddNode<Multiply<Tensor>>("FC1", {"ContextVectors", "FC"});

  // Apply elementwise Tanh
  g.AddNode<TanH<Tensor>>("TanH", {"FC1"});

  // Calculate the contexts_weights, i.e. \sum_{i} TanH_{i,j} * attention_param_{j} yielding a
  // vector of size (batch_size, 1)
  // The attention_param is of size (embedding_size * 3)
  g.AddNode<Multiply<Tensor>>("ContextWeights", {"TanH", "AttentionParameters"});

  // Apply Softmax on it to get the attention_weights (the sum in the denominator runs over the
  // batch)
  g.AddNode<SoftMax<Tensor>>("AttentionWeights", {"ContextWeights"});

  // Make the codevectors, i.e. reduce_sum(multiply(attentionweights, tanh))
  // Result has shape (embedding_size*3)
  g.AddNode<Multiplay<Tensor>>("CV1_1", {"AttentionWeights", "TanH"});
  g.AddNode<ReduceSum<Tensor>>("CV1_2", {"CV1_1"});

  // Multiplying the embedding of the target, with the code vectors (gives an object of shape
  // (3*embedding_size, 3*embedding_size) )
  g.AddNode<Multiply<Tensor>>("Logits", {"TargetWordEmbedding", "CV1"});

  // Applying softmax_cross_entropy_with_logits
  g.AddNode<SoftMax<Tensor>>("Probabilities", {"Logits"});
  g.AddNode<Log<Tensor>>("LogProbabilities", {"Probabilities"});
  g.AddNode<RetrieveRows<Tensor>>("CrossEntropy", {"LogProbabilities", "TargetInput"});

  // Sum over values
  g.AddNode<ReduceSum<ArrayType>>("Result", {"CrossEntropy"});

  // Batch Loop
  for (auto &batch : ...)
  {
    g.BackPropagate("Result", ...);
  }
}