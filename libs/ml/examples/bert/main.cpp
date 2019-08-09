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
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/add.hpp"
#include "ml/layers/self_attention_encoder.hpp"
#include "ml/optimisation/adam_optimiser.hpp"

#include <iostream>
#include <string>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType  = float;
using ArrayType = fetch::math::Tensor<DataType>;
using SizeType  = typename ArrayType::SizeType;

using GraphType      = typename fetch::ml::Graph<ArrayType>;
using OptimiserType  = typename fetch::ml::optimisers::AdamOptimiser<ArrayType>;

int main(int ac, char **av)
{

  if (ac < 3)
  {
    std::cout << "Usage : " << av[0]
              << " PATH/TO/train-images-idx3-ubyte PATH/TO/train-labels-idx1-ubyte" << std::endl;
    return 1;
  }

  std::cout << "FETCH BERT Demo" << std::endl;
  
  // set up input shape
  SizeType n_encoder_layers = 12u;
  SizeType max_seq_len = 512u;
  SizeType model_dims = 768u;
  SizeType n_heads = 12u;
  SizeType ff_dims = 4u * model_dims;
  DataType dropout_keep_prob = static_cast<DataType>(0.9);
  
  // Prepare input with segment embedding, position embedding, token embedding and masking
	fetch::ml::Graph<ArrayType> g;
	std::string segment = g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Segment", {});
	std::string position = g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Position", {});
	std::string tokens = g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Tokens", {});
	std::string mask  = g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Mask", {});
	
	// prepare embedding for segment, position and tokens
	std::string segment_embedding = g.template AddNode<fetch::ml::ops::Embeddings<ArrayType>>("Segment_Embedding", {segment}, static_cast<SizeType>(model_dims), static_cast<SizeType>(2));
	std::string position_embedding = g.template AddNode<fetch::ml::ops::Embeddings<ArrayType>>("Position_Embedding", {position}, static_cast<SizeType>(model_dims), static_cast<SizeType>(max_seq_len));
	std::string token_embedding = g.template AddNode<fetch::ml::ops::Embeddings<ArrayType>>("Token_Embedding", {tokens}, static_cast<SizeType>(model_dims), static_cast<SizeType>(6000));
 
	// summ embedding together
	std::string seg_pos_add = g.template AddNode<fetch::ml::ops::Add<ArrayType>>("seg_pos_add", {segment_embedding, position_embedding});
	std::string sum_input = g.template AddNode<fetch::ml::ops::Add<ArrayType>>("all_input_add", {token_embedding, seg_pos_add});
	
  // Ensemble the whole bert model
  std::string layer_output = sum_input;
  for(SizeType i=0u; i<n_encoder_layers; i++){
	  layer_output = g.template AddNode<fetch::ml::layers::SelfAttentionEncoder<ArrayType>>(
	   "SelfAttentionEncoder_No_" + std::to_string(i), {layer_output, mask}, n_heads, model_dims, ff_dims, dropout_keep_prob);
  }
	
  
  
  // Create a sudo data for input

  // Prepare graph
  //  Input -> FC -> Relu -> FC -> Relu -> FC -> Softmax
  auto g = std::make_shared<GraphType>();

  std::string input = g->AddNode<PlaceHolder<ArrayType>>("Input", {});
  std::string label = g->AddNode<PlaceHolder<ArrayType>>("Label", {});

  std::string layer_1 = g->AddNode<FullyConnected<ArrayType>>(
      "FC1", {input}, 28u * 28u, 10u, fetch::ml::details::ActivationType::RELU, regulariser,
      reg_rate);
  std::string layer_2 = g->AddNode<FullyConnected<ArrayType>>(
      "FC2", {layer_1}, 10u, 10u, fetch::ml::details::ActivationType::RELU, regulariser, reg_rate);
  std::string output = g->AddNode<FullyConnected<ArrayType>>(
      "FC3", {layer_2}, 10u, 10u, fetch::ml::details::ActivationType::SOFTMAX, regulariser,
      reg_rate);
  std::string error = g->AddNode<CrossEntropyLoss<ArrayType>>("Error", {output, label});

  // Initialise MNIST loader
  DataLoaderType data_loader(av[1], av[2]);

  // Initialise Optimiser
  OptimiserType optimiser(g, {input}, label, error, learning_rate);

  // Training loop
  DataType loss;
  for (SizeType i{0}; i < epochs; i++)
  {
    loss = optimiser.Run(data_loader, batch_size, subset_size);
    std::cout << "Loss: " << loss << std::endl;
  }

  return 0;
}
