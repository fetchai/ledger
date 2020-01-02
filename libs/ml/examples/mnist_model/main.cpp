//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "ml/dataloaders/tensor_dataloader.hpp"
#include "ml/model/dnn_classifier.hpp"
#include "ml/optimisation/types.hpp"
#include "ml/utilities/mnist_utilities.hpp"

#include <iostream>

using namespace fetch::ml::model;
using namespace fetch::ml::optimisers;

using DataType   = fetch::fixed_point::FixedPoint<32, 32>;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType   = fetch::math::SizeType;

using ModelType      = typename fetch::ml::model::DNNClassifier<TensorType>;
using DataLoaderType = typename fetch::ml::dataloaders::TensorDataLoader<TensorType, TensorType>;
using OptimiserType  = fetch::ml::OptimiserType;

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
  ModelConfig<DataType> model_config;
  model_config.learning_rate_param.mode =
      fetch::ml::optimisers::LearningRateParam<DataType>::LearningRateDecay::EXPONENTIAL;
  model_config.learning_rate_param.starting_learning_rate = fetch::math::Type<DataType>("0.001");
  model_config.learning_rate_param.exponential_decay_rate = fetch::math::Type<DataType>("0.99");
  model_config.batch_size                                 = 64;  // minibatch training size
  model_config.subset_size         = 1000;  // train on 1000 samples then run tests/save graph
  model_config.early_stopping      = true;  // stop early if no improvement
  model_config.patience            = 30;
  model_config.print_stats         = true;
  model_config.save_graph          = true;
  model_config.graph_save_location = "/tmp/mnist_model";

  // setup dataloader with 20% test set
  auto mnist_images = fetch::ml::utilities::read_mnist_images<TensorType>(av[1]);
  auto mnist_labels = fetch::ml::utilities::read_mnist_labels<TensorType>(av[2]);
  mnist_labels      = fetch::ml::utilities::convert_labels_to_onehot(mnist_labels);

  auto data_loader_ptr = std::make_unique<DataLoaderType>();
  data_loader_ptr->AddData({mnist_images}, mnist_labels);
  data_loader_ptr->SetTestRatio(0.2f);

  // setup model and pass dataloader
  ModelType model(model_config, {784, 100, 20, 10});
  model.SetDataloader(std::move(data_loader_ptr));
  model.Compile(OptimiserType::ADAM);

  // training loop - early stopping will prevent long training time
  DataType loss;
  model.Train(1000000, loss);

  // Run model on a test set
  DataType test_loss;
  model.Test(test_loss);

  std::cout << "The training has finished, validation loss: " << test_loss << std::endl;

  return 0;
}
