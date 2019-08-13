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
#include "ml/layers/self_attention_encoder.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/slice.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/optimisation/adam_optimiser.hpp"

#include <iostream>
#include <string>
#include <chrono>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType  = float;
using ArrayType = fetch::math::Tensor<DataType>;
using SizeType  = typename ArrayType::SizeType;

using GraphType     = typename fetch::ml::Graph<ArrayType>;
using OptimiserType = typename fetch::ml::optimisers::AdamOptimiser<ArrayType>;

using RegType         = fetch::ml::details::RegularisationType;
using WeightsInitType = fetch::ml::ops::WeightsInitialisation;
using ActivationType  = fetch::ml::details::ActivationType;

ArrayType create_position_data(SizeType max_seq_len, SizeType batch_size);
ArrayType create_mask_data(SizeType max_seq_len, ArrayType seq_len_per_batch);
std::pair<std::vector<ArrayType>, ArrayType> prepare_data_for_simple_cls(SizeType max_seq_len, SizeType batch_size);

int main(/*int ac, char **av*/)
{
//
//  if (ac < 3)
//  {
//    std::cout << "Usage : " << av[0]
//              << " PATH/TO/train-images-idx3-ubyte PATH/TO/train-labels-idx1-ubyte" << std::endl;
//    return 1;
//  }

  std::cout << "FETCH BERT Demo" << std::endl;

  // set up input shape
  
// small inputs
  SizeType n_encoder_layers  = 12u;
  SizeType max_seq_len       = 10u;//512u;
  SizeType model_dims        = 8u;//768u;
  SizeType n_heads           = 2u;//12u;
  SizeType ff_dims           = 10u;//4u * model_dims;
  SizeType vocab_size        = 3u;//6000u;
  SizeType segment_size      = 2u;
  
  ArrayType a = ArrayType::FromString("1e-5, 1e-6;7e-5, 7e-6;");

// big inputs
//	SizeType n_encoder_layers  = 12u;
//	SizeType max_seq_len       = 512u;
//	SizeType model_dims        = 768u;
//	SizeType n_heads           = 12u;
//	SizeType ff_dims           = 4u * model_dims;
//	SizeType vocab_size        = 10000u;
//	SizeType segment_size      = 2u;
  DataType dropout_keep_prob = static_cast<DataType>(0.9);

  // Prepare input with segment embedding, position embedding, token embedding and masking
  fetch::ml::Graph<ArrayType> g;
  std::string segment  = g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Segment", {});
  std::string position = g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Position", {});
  std::string tokens   = g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Tokens", {});
  std::string mask     = g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Mask", {});

  // prepare embedding for segment, position and tokens
  std::string segment_embedding = g.template AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
      "Segment_Embedding", {segment}, static_cast<SizeType>(model_dims), static_cast<SizeType>(segment_size));
  std::string position_embedding = g.template AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
      "Position_Embedding", {position}, static_cast<SizeType>(model_dims),
      static_cast<SizeType>(max_seq_len));
  std::string token_embedding = g.template AddNode<fetch::ml::ops::Embeddings<ArrayType>>(
      "Token_Embedding", {tokens}, static_cast<SizeType>(model_dims), static_cast<SizeType>(vocab_size));

  // sum embedding together
  std::string seg_pos_add = g.template AddNode<fetch::ml::ops::Add<ArrayType>>(
      "seg_pos_add", {segment_embedding, position_embedding});
  std::string sum_input = g.template AddNode<fetch::ml::ops::Add<ArrayType>>(
      "all_input_add", {token_embedding, seg_pos_add});

  // Ensemble the whole bert model
  std::string layer_output = sum_input;
  for (SizeType i = 0u; i < n_encoder_layers; i++)
  {
    layer_output = g.template AddNode<fetch::ml::layers::SelfAttentionEncoder<ArrayType>>(
        "SelfAttentionEncoder_No_" + std::to_string(i), {layer_output, mask}, n_heads, model_dims,
        ff_dims, dropout_keep_prob);
  }

  // Add linear classification layer
  std::string cls_token_output = g.template AddNode<fetch::ml::ops::Slice<ArrayType>>("ClsTokenOutput", {layer_output}, 0u, 1u);
  std::string classification_output = g.template AddNode<fetch::ml::layers::FullyConnected<ArrayType>>("ClassificationOutput", {cls_token_output}, model_dims,
                                                                                                       1u, ActivationType::SIGMOID, RegType::NONE,
                                                                                                       static_cast<DataType>(0), WeightsInitType::XAVIER_GLOROT, false);
  // Set up error signal
	std::string label = g.template AddNode<PlaceHolder<ArrayType>>("Label", {});
	std::string error = g.template AddNode<CrossEntropyLoss<ArrayType>>("Error", {classification_output, label});
	
	// Initialise Optimiser
	fetch::ml::optimisers::AdamOptimiser<ArrayType> optimiser(std::make_shared<fetch::ml::Graph<ArrayType>>(g), {segment, position, tokens, mask}, label,
	                                                          error, static_cast<DataType>(1e-3));
	
//	SizeType batch_size = 10u;
//	SizeType seq_len = 100u;
//
//	ArrayType tokens_data({max_seq_len, batch_size});
//	ArrayType mask_data({max_seq_len, max_seq_len, batch_size});
//	for(SizeType i=0; i<seq_len; i++){
//		for(SizeType t=0; t<seq_len; t++){
//			for(SizeType b=0; b<batch_size; b++){
//				mask_data.Set(i, t, b, static_cast<DataType>(1));
//			}
//		}
//	}
//	ArrayType position_data({max_seq_len, batch_size});
//	for(SizeType i=0; i<seq_len; i++){
//		for(SizeType b=0; b<batch_size; b++){
//			position_data.Set(i, b, static_cast<DataType>(i));
//		}
//	}
//	ArrayType segment_data({max_seq_len, batch_size});
//	g.SetInput(segment, segment_data);
//	g.SetInput(position, position_data);
//	g.SetInput(tokens, tokens_data);
//	g.SetInput(mask, mask_data);
	
//	std::cout << "Starting forward passing" << std::endl;
//	auto cur_time  = std::chrono::high_resolution_clock::now();
//	auto output = g.Evaluate(classification_output, false);
//	auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - cur_time);
//	std::cout << "time span: " << static_cast<double>(time_span.count()) << std::endl;
//	std::cout << "output.ToString(): " << output.ToString() << std::endl;

	auto train_data = prepare_data_for_simple_cls(max_seq_len, 30u);
	for(SizeType i=0; i<100; i++){
		DataType loss = optimiser.Run(train_data.first, train_data.second);
		std::cout << "loss: " << loss << std::endl;
	}
	
	std::cout << "Starting forward passing for manual evaluation" << std::endl;
	ArrayType segment_data = train_data.first[0];
	ArrayType position_data = train_data.first[1];
	ArrayType tokens_data = train_data.first[2];
	ArrayType mask_data = train_data.first[3];
	g.SetInput(segment, segment_data);
	g.SetInput(position, position_data);
	g.SetInput(tokens, tokens_data);
	g.SetInput(mask, mask_data);
	auto output = g.Evaluate(classification_output, false);
	std::cout << "model output: " << output.ToString() << std::endl;
	std::cout << "label output: " << train_data.second.ToString() << std::endl;
	
	return 0;
}



std::pair<std::vector<ArrayType>, ArrayType> prepare_data_for_simple_cls(SizeType max_seq_len, SizeType batch_size){
	ArrayType segment_data({max_seq_len, batch_size});
	ArrayType position_data = create_position_data(max_seq_len, batch_size);
	ArrayType token_data({max_seq_len, batch_size});
	ArrayType mask_data({max_seq_len, max_seq_len, batch_size});
	ArrayType labels({1u, batch_size});
	mask_data.Fill(static_cast<DataType>(1));
	
	for(SizeType i=0; i<batch_size; i++){
		if(i%4 == 0){ // all 1
			for(SizeType entry=1; entry<max_seq_len; entry++){
				token_data.Set(entry, i, static_cast<DataType>(1));
			}
			labels.Set(0u, i, static_cast<DataType>(0));
		}else if(i%4 == 2){ // all 2
			for(SizeType entry=1; entry<max_seq_len; entry++){
				token_data.Set(entry, i, static_cast<DataType>(2));
			}
			labels.Set(0u, i, static_cast<DataType>(0));
		}else{ // interval data
			for(SizeType entry=1; entry<max_seq_len; entry++){
				if(entry%2==1){
					token_data.Set(entry, i, static_cast<DataType>(1));
				}else{
					token_data.Set(entry, i, static_cast<DataType>(2));
				}
			}
			labels.Set(0u, i, static_cast<DataType>(1));
		}
	}
	
	return std::make_pair(std::vector<ArrayType>({segment_data, position_data, token_data, mask_data}), labels);
}

ArrayType create_position_data(SizeType max_seq_len, SizeType batch_size){
	ArrayType ret_position({max_seq_len, batch_size});
	for(SizeType i=0; i<max_seq_len; i++){
		for(SizeType b=0; b<batch_size; b++){
			ret_position.Set(i, b, static_cast<DataType>(i));
		}
	}
	return ret_position;
}

ArrayType create_mask_data(SizeType max_seq_len, ArrayType seq_len_per_batch){
	assert(seq_len_per_batch.shape().size() == 2);
	assert(fetch::math::Max(seq_len_per_batch) <= static_cast<DataType>(max_seq_len));
	SizeType batch_size = seq_len_per_batch.shape(0);
	ArrayType ret_mask({max_seq_len, max_seq_len, batch_size});
	for(SizeType b=0; b<batch_size; b++){
		auto seq_len = static_cast<SizeType>(seq_len_per_batch.At(0, b));
		for(SizeType i=0; i<seq_len; i++) {
			for (SizeType t = 0; t < seq_len; t++) {
				ret_mask.Set(i, t, b, static_cast<DataType>(1));
			}
		}
	}
	return ret_mask;
}