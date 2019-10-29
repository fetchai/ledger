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

#include "bert_utilities.hpp"
#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/serializers/ml_types.hpp"

#include <iostream>
#include <string>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType   = float;
using TensorType = fetch::math::Tensor<DataType>;
using SizeType   = typename TensorType::SizeType;
using SizeVector = typename TensorType::SizeVector;

using GraphType     = typename fetch::ml::Graph<TensorType>;
using StateDictType = typename fetch::ml::StateDict<TensorType>;
using OptimiserType = typename fetch::ml::optimisers::AdamOptimiser<TensorType>;

using RegType         = fetch::ml::RegularisationType;
using WeightsInitType = fetch::ml::ops::WeightsInitialisation;
using ActivationType  = fetch::ml::details::ActivationType;

int main(int ac, char **av)
{
  // the example takes in two arguments: first for pretrained bert txt folder, second for bin file
  // to save model
  // for the pretrained bert txt folder, you can generate this txt weight files with the relevant
  // scripts in https://github.com/uvue-git/fetch-ledger-test-scripts
  if (ac != 3)
  {
    std::cout << "Wrong number of / No available argument" << std::endl;
    return 1;
  }

  // from and to dir
  std::string pretrained_model_dir = av[1];
  std::string saved_model_path     = av[2];

  // load pretrained bert model and print its output of a toy input
  BERTConfig    config;
  BERTInterface interface(config);
  auto *        g = new GraphType();

  std::cout << "load pretrained pytorch bert model from folder: \n"
            << pretrained_model_dir << std::endl;
  LoadPretrainedBertModel(pretrained_model_dir, config, *g);

  std::cout << "get an output for the bert loaded from txt files" << std::endl;
  TensorType first_output =
      RunPseudoForwardPass(interface.inputs, interface.outputs[interface.outputs.size() - 1],
                           config, *g, static_cast<SizeType>(1), false);

  std::cout << "save the pretrained bert model to file: \n" << saved_model_path << std::endl;
  SaveGraphToFile(*g, saved_model_path);

  // delete the model for memory efficiency
  delete g;

  std::cout << "load saved model for testing" << std::endl;
  GraphType g2 = ReadFileToGraph(saved_model_path);

  std::cout << "get another output for the bert loaded from bin file" << std::endl;
  TensorType second_output =
      RunPseudoForwardPass(interface.inputs, interface.outputs[interface.outputs.size() - 1],
                           config, g2, static_cast<SizeType>(1), false);

  if (first_output == second_output)
  {
    std::cout << "The saved model matched the origin model, congrats!!!" << std::endl;
  }
  else
  {
    fetch::ml::exceptions::InvalidMode("The serialization is not working properly");
  }

  return 0;
}
