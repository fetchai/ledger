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

#include "../../examples/distributed_learning/distributed_learning_client.hpp"
#include "core/random.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <utility>
#include <vector>

using DataType         = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType       = fetch::math::Tensor<DataType>;
using VectorTensorType = std::vector<TensorType>;
using SizeType         = fetch::math::SizeType;

#define BATCH_SIZE 128
#define LEARNING_RATE .001f
#define TEST_SET_RATIO 0.03f
#define NUMBER_OF_PEERS 3

enum class CoordinatorState
{
  RUN,
  STOP,
};

class Coordinator
{
public:
  CoordinatorState state = CoordinatorState::RUN;
};

class TrainingClient
{
public:
  TrainingClient(std::string const &images, std::string const &labels, std::string const &id);
  void             SetCoordinator(std::shared_ptr<Coordinator> coordinator_ptr);
  void             MainLoop();
  DataType         Train();
  void             Test(DataType &test_loss);
  VectorTensorType GetGradients() const;
  VectorTensorType GetWeights() const;
  void             AddPeers(std::vector<std::shared_ptr<TrainingClient>> const &clients);
  void             BroadcastGradients();
  void             AddGradient(VectorTensorType gradient);
  void             ApplyGradient(VectorTensorType gradients);
  void             SetWeights(VectorTensorType &new_weights);

private:
  // Client's own graph and mutex to protect it's weights
  fetch::ml::Graph<TensorType> g_;
  std::mutex                   model_mutex_;

  // Client's own dataloader
  fetch::ml::dataloaders::MNISTLoader<TensorType, TensorType> dataloader_;

  // Connection to other nodes
  std::vector<std::shared_ptr<TrainingClient>> peers_;

  // Access to coordinator
  std::shared_ptr<Coordinator> coordinator_ptr_;

  // Gradient queue access and mutex to protect it
  std::queue<VectorTensorType> gradient_queue_;
  std::mutex                   queue_mutex_;

  // random number generator for shuffling peers
  fetch::random::LaggedFibonacciGenerator<> gen_;

  // Learning hyperparameters
  SizeType batch_size_      = BATCH_SIZE;
  float    test_set_ratio_  = TEST_SET_RATIO;
  SizeType number_of_peers_ = NUMBER_OF_PEERS;
  DataType learning_rate_   = static_cast<DataType>(LEARNING_RATE);

  // Client id (identification name)
  std::string id_;

  void        GetNewGradients(VectorTensorType &new_gradients);
  std::string GetTimeStamp();
};
