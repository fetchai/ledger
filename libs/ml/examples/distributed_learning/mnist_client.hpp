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

#include "ml/optimisation/adam_optimiser.hpp"
#include <distributed_learning_client.hpp>

template <class TensorType>
class MNISTClient : public TrainingClient<TensorType>
{
  using DataType = typename TensorType::Type;
  using SizeType = typename TensorType::SizeType;

public:
  MNISTClient(std::string const &images, std::string const &labels, std::string const &id,
              SizeType batch_size, DataType learning_rate, float test_set_ratio,
              SizeType number_of_peers)
    : TrainingClient<TensorType>(id, batch_size, learning_rate, test_set_ratio, number_of_peers)
    , images_(std::move(images))
    , labels_(std::move(labels))
    , test_set_ratio_(test_set_ratio)
  {
    PrepareDataLoader();
    PrepareModel();
    PrepareOptimiser();
  }

  void PrepareModel() override
  {
    this->g_ptr_ = std::make_shared<fetch::ml::Graph<TensorType>>();
    this->g_     = *this->g_ptr_;

    this->inputs_names_ = {this->g_.template AddNode<PlaceHolder<TensorType>>("Input", {})};
    this->g_.template AddNode<FullyConnected<TensorType>>("FC1", {"Input"}, 28u * 28u, 10u);
    this->g_.template AddNode<Relu<TensorType>>("Relu1", {"FC1"});
    this->g_.template AddNode<FullyConnected<TensorType>>("FC2", {"Relu1"}, 10u, 10u);
    this->g_.template AddNode<Relu<TensorType>>("Relu2", {"FC2"});
    this->g_.template AddNode<FullyConnected<TensorType>>("FC3", {"Relu2"}, 10u, 10u);
    this->g_.template AddNode<Softmax<TensorType>>("Softmax", {"FC3"});
    this->label_name_ = this->g_.template AddNode<PlaceHolder<TensorType>>("Label", {});
    this->error_name_ =
        this->g_.template AddNode<CrossEntropyLoss<TensorType>>("Error", {"Softmax", "Label"});
  }

  void PrepareDataLoader() override
  {
    this->dataloader_ptr_ =
        std::make_shared<fetch::ml::dataloaders::MNISTLoader<TensorType, TensorType>>(images_,
                                                                                      labels_);
    this->dataloader_ptr_->SetTestRatio(test_set_ratio_);
    this->dataloader_ptr_->SetRandomMode(true);
  }

  void PrepareOptimiser() override
  {
    // Initialise Optimiser
    this->opti_ptr_ = std::make_shared<fetch::ml::optimisers::AdamOptimiser<TensorType>>(
        std::shared_ptr<fetch::ml::Graph<TensorType>>(this->g_ptr_), this->inputs_names_,
        this->label_name_, this->error_name_, this->learning_rate_);
  }

private:
  std::string const images_;
  std::string const labels_;
  float             test_set_ratio_;
};
