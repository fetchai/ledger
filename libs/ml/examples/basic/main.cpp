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
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/cross_entropy.hpp"

#include "mnist_loader.hpp"

#include <iostream>

using namespace fetch::ml::ops;
using namespace fetch::ml::layers;

using DataType  = float;
using ArrayType = fetch::math::Tensor<DataType>;

int main()
{
  std::cout << "FETCH MNIST Demo" << std::endl;
  MNISTLoader                 dataloader;
  fetch::ml::Graph<ArrayType> g;

  g.AddNode<PlaceHolder<ArrayType>>("Input", {});
  g.AddNode<FullyConnected<ArrayType>>("FC1", {"Input"}, 28u * 28u, 10u);
  g.AddNode<Relu<ArrayType>>("Relu1", {"FC1"});
  g.AddNode<FullyConnected<ArrayType>>("FC2", {"Relu1"}, 10u, 10u);
  g.AddNode<Relu<ArrayType>>("Relu2", {"FC1"});
  g.AddNode<FullyConnected<ArrayType>>("FC3", {"Relu2"}, 10u, 10u);
  g.AddNode<Softmax<ArrayType>>("Softmax", {"FC3"});
  //  Input -> FC -> Relu -> FC -> Relu -> FC -> Softmax

  CrossEntropyLayer<ArrayType> criterion;
  //  MeanSquareErrorLayer<ArrayType> criterion;

  std::pair<size_t, std::shared_ptr<ArrayType>> input;
  std::shared_ptr<ArrayType>                    gt =
      std::make_shared<ArrayType>(std::vector<typename ArrayType::SizeType>({1, 10}));

  gt->At(0)         = 1.0;
  DataType     loss = 0;
  unsigned int i(0);

  while (true)
  {
    if (dataloader.IsDone())
    {
      dataloader.Reset();
    }
    input = dataloader.GetNext(input.second);
    g.SetInput("Input", input.second);
    gt->Fill(0);
    gt->At(input.first)                = DataType(1.0);
    std::shared_ptr<ArrayType> results = g.Evaluate("Softmax");

    loss += criterion.Forward({results, gt});
    g.BackPropagate("Softmax", criterion.Backward({results, gt}));
    i++;
    if (i % 60 == 0)
    {
      std::cout << "MiniBatch: " << i / 60 << " -- Loss : " << loss << std::endl;
      g.Step(0.01f);
      loss = 0;
    }
  }
  return 0;
}
