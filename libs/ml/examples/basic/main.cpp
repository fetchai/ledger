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
#include "ml/ops/cross_entropy.hpp"
#include "ml/ops/fully_connected.hpp"
#include "ml/ops/mean_square_error.hpp"
#include "ml/ops/relu.hpp"
#include "ml/ops/softmax.hpp"

#include "mnist_loader.hpp"

#include <iostream>

using namespace fetch::ml::ops;

int main()
{
  std::cout << "FETCH MNIST Demo" << std::endl;
  using Datatype = fetch::math::Tensor<float>;
  MNISTLoader                dataloader;
  fetch::ml::Graph<Datatype> g;
  g.AddNode<PlaceHolder<Datatype>>("Input", {});
  g.AddNode<FullyConnected<Datatype>>("FC1", {"Input"}, 28u * 28u, 10u);
  g.AddNode<ReluLayer<Datatype>>("Relu1", {"FC1"});
  g.AddNode<FullyConnected<Datatype>>("FC2", {"Relu1"}, 10u, 10u);
  g.AddNode<ReluLayer<Datatype>>("Relu2", {"FC1"});
  g.AddNode<FullyConnected<Datatype>>("FC3", {"Relu2"}, 10u, 10u);
  g.AddNode<SoftmaxLayer<Datatype>>("Softmax", {"FC3"});
  //  Input -> FC -> Relu -> FC -> Relu -> FC -> Softmax

  MeanSquareErrorLayer<Datatype> criterion;

  std::pair<size_t, std::shared_ptr<Datatype>> input;
  std::shared_ptr<Datatype> gt = std::make_shared<Datatype>(std::vector<size_t>({10}));

  gt->At(0)         = 1.0;
  float        loss = 0;
  unsigned int i(0);
  while (true)
  {
    if (dataloader.IsDone())
    {
      dataloader.Reset();
    }
    input = dataloader.GetNext(input.second);
    gt->Fill(0);
    gt->At(input.first) = 1.0f;
    g.SetInput("Input", input.second);
    //      dataloader.Display(input.second);
    std::shared_ptr<Datatype> results = g.Evaluate("Softmax");
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
