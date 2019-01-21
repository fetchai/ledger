#include "math/tensor.hpp"
#include "ml/graph.hpp"
#include "ml/ops/fully_connected.hpp"
#include "ml/ops/relu.hpp"
#include "ml/ops/softmax.hpp"

#include <iostream>

using namespace fetch::ml::ops;

int main(int ac, char **av)
{
  std::cout << "FETCH MNIST Demo" << std::endl;
  
  typedef fetch::math::Tensor<float> Datatype;

  fetch::ml::Graph<Datatype> g;
  g.AddNode<PlaceHolder<Datatype>>("Input", {});
  g.AddNode<FullyConnected<Datatype>>("FC1", {"Input"}, 28u * 28u, 100u);
  g.AddNode<ReluLayer<Datatype>>("Relu1", {"FC1"});
  g.AddNode<FullyConnected<Datatype>>("FC2", {"Relu1"}, 100u, 100u);
  g.AddNode<ReluLayer<Datatype>>("Relu2", {"FC1"});
  g.AddNode<FullyConnected<Datatype>>("FC3", {"Relu2"}, 100u, 10u);
  g.AddNode<SoftmaxLayer<Datatype>>("Softmax", {"FC3"});
  //  Input -> FC -> Relu -> FC -> Relu -> FC -> Softmax


  std::shared_ptr<Datatype> input = std::make_shared<Datatype>(std::vector<size_t>({28, 28}));
  for (size_t i(0) ; i < 28 ; ++i)
    {
      input->Set(std::vector<size_t>({i, 12}), 1.0f);
    }

  g.SetInput("Input", input);

  std::shared_ptr<Datatype> result2 = g.Evaluate("Softmax");
  std::cout << "Res2:\n" << result2->ToString() << std::endl;


  
  return 0;
}
