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

//#include "math/tensor.hpp"
//#include "ml/graph.hpp"
//#include "ml/layers/fully_connected.hpp"
//#include "ml/ops/activation.hpp"
//#include "ml/ops/loss_functions/cross_entropy.hpp"
//#include "ml/optimisation/adam_optimiser.hpp"

#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/estimators/dnn_classifier.hpp"
#include "ml/optimisation/types.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace fetch::ml::estimator;
using namespace fetch::ml::optimisers;

using DataType   = float;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType   = typename TensorType::SizeType;

using EstimatorType  = typename fetch::ml::estimator::DNNClassifier<TensorType>;
using DataLoaderType = typename fetch::ml::MNISTLoader<TensorType, TensorType>;

int main(int ac, char **av)
{
  if (ac < 3)
  {
    std::cout << "Usage : " << av[0]
              << " PATH/TO/train-images-idx3-ubyte PATH/TO/train-labels-idx1-ubyte" << std::endl;
    return 1;
  }

  std::cout << "FETCH MNIST Demo" << std::endl;

  // setup config
  EstimatorConfig<DataType> estimator_config;
  estimator_config.batch_size     = 64;
  estimator_config.early_stopping = true;
  estimator_config.learning_rate  = 0.5;
  estimator_config.opt            = OptimiserType::ADAM;

  // setup dataloader - TODO: make this go away by implementing default dataloader in estimator
  auto data_loader_ptr = std::make_shared<DataLoaderType>(av[1], av[2]);

  // run estimator
  EstimatorType estimator(estimator_config, data_loader_ptr);
  estimator.Run(1000, RunMode::TRAIN);

  return 0;
}
