#pragma once
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

#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/weights.hpp"
#include "ml/ops/layer_norm.hpp"
#include "ml/regularisers/regularisation.hpp"
#include "ml/regularisers/regulariser.hpp"
#include "ml/subgraph.hpp"

#include <cmath>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class LayerNorm: public SubGraph<T>
{
public:
  using ArrayType    = T;
  using TensorPtrType = std::shared_ptr<ArrayType>;
  using SizeType      = typename ArrayType::SizeType;
  using DataType      = typename ArrayType::Type;
  using WeightsInit   = fetch::ml::ops::WeightsInitialisation;

  LayerNorm(std::vector<SizeType> const &data_shape, DataType epsilon = fetch::math::numeric_lowest<DataType>())
    : data_shape_(data_shape),
    epsilon_(epsilon)
  {
  	// the data_shape is the shape of the data without including the batch dims
		// currently 1D input or 1D inputs with time dims is supported
		// currently, we only allow layernorm to be down in the first dims
  	ASSERT(data_shape.size() <= 2);
  	
  	std::string name = DESCRIPTOR;
    std::string input = this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input", {});

    // instantiate gamma and beta (the multiplicative training component)
    std::string gamma = this->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Gamma", {});
    std::string beta = this->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Beta", {});
    ArrayType gamma_data, beta_data;
    if(data_shape_.size() == 1){
    	gamma_data = ArrayType({data_shape_.at(0), 1});
    	beta_data = ArrayType({data_shape_.at(0), 1});
    }else if(data_shape_.size() == 2){
    	gamma_data = ArrayType({data_shape_.at(0), 1, 1});
	    beta_data = ArrayType({data_shape_.at(0), 1, 1});
    }else{
    	throw std::runtime_error("more than 3D input data cannot be handled yet");
    }
	  gamma_data.Fill(static_cast<DataType>(1));
    this->SetInput(gamma, gamma_data);
	  this->SetInput(beta, beta_data);
	  
	  // do the normalization
    std::string output = this->template AddNode<fetch::ml::ops::LayerNorm<ArrayType>>(
        name + "_BatchNorm", {input, gamma, beta}, data_shape_);
		
    // do the rescaling
    std::string
    
    this->AddInputNode(input);
    this->SetOutputNode(output);
  }

  std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs) const
  {
    return {inputs.at(0).get().shape()};
  }
	
	static constexpr char const *DESCRIPTOR = "LayerNorm";

private:
  std::vector<SizeType> data_shape_;
  DataType epsilon_;
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
