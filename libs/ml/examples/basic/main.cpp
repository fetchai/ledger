#include "math/tensor.hpp"
#include "ml/graph.hpp"
#include "ml/ops/cross_entropy.hpp"
#include "ml/ops/fully_connected.hpp"
#include "ml/ops/mean_square_error.hpp"
#include "ml/ops/relu.hpp"
#include "ml/ops/softmax.hpp"

#include <iostream>

using namespace fetch::ml::ops;

int main(int ac, char **av)
{
  std::cout << "FETCH MNIST Demo" << std::endl;

  std::cout << std::setprecision(3) << std::endl;
  
  typedef fetch::math::Tensor<float> Datatype;

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

  std::shared_ptr<Datatype> input = std::make_shared<Datatype>(std::vector<size_t>({28, 28}));
  std::shared_ptr<Datatype> gt = std::make_shared<Datatype>(std::vector<size_t>({10}));
  for (size_t i(0) ; i < 28 ; ++i)
    {
      input->Set(std::vector<size_t>({i, 12}), 1.0f);
    }
  gt->At(0) = 1.0;
  g.SetInput("Input", input);
  float loss = 1;
  while (loss > 0)
    {
      std::shared_ptr<Datatype> results = g.Evaluate("Softmax");
      loss = criterion.Forward({results, gt});
      g.BackPropagate("Softmax", criterion.Backward({results, gt}));
      g.Step();
      std::cout << "Res: " << results->ToString() << " -- Loss : " << loss << std::endl;      
    }

  return 0;
}
