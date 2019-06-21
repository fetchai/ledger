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

#include "ml/dataloaders/mnist_loaders/mnist_loader.hpp"
#include "ml/estimators/dnn_classifier.hpp"
#include "ml/optimisation/types.hpp"

using namespace fetch::ml::estimator;
using namespace fetch::ml::optimisers;

using DataType   = float;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType   = typename TensorType::SizeType;

using EstimatorType  = typename fetch::ml::estimator::DNNClassifier<TensorType>;
using DataLoaderType = typename fetch::ml::dataloaders::MNISTLoader<TensorType, TensorType>;

// TODO: add an estimator test

int main(int ac, char **av)
{
  if (ac < 3)
  {
    std::cout << "Usage : " << av[0]
              << " PATH/TO/train-images-idx3-ubyte PATH/TO/train-labels-idx1-ubyte" << std::endl;
    return 1;
  }

  std::cout << "FETCH MNIST Demo" << std::endl;

  /// setup config ///
  EstimatorConfig<DataType> estimator_config;
  estimator_config.learning_rate  = 0.01f;
  estimator_config.lr_decay       = 0.99f;
  estimator_config.batch_size     = 64;
  estimator_config.subset_size    = 1000;
  estimator_config.early_stopping = true;
  estimator_config.opt            = OptimiserType::ADAM;

  // setup dataloader
  auto       data_loader_ptr = std::make_shared<DataLoaderType>(av[1], av[2]);
  TensorType test_label      = (data_loader_ptr->GetNext()).second.at(0);
  TensorType test_input      = (data_loader_ptr->GetNext()).second.at(0);
  TensorType prediction;
  DataType   loss;

  // run estimator in training mode
  EstimatorType estimator(estimator_config, data_loader_ptr, {784, 100, 20, 10});

  for (std::size_t i = 0; i < 10; ++i)
  {
    estimator.Train(100, loss);
    std::cout << "epoch: " << (i*100) << ", loss: " << loss << std::endl;
  }

  // run estimator in testing mode
  estimator.Predict(test_input, prediction);
  std::cout << "test_label.ToString(): " << test_label.ToString() << std::endl;
  std::cout << "prediction.ToString(): " << prediction.ToString() << std::endl;

  return 0;
}
