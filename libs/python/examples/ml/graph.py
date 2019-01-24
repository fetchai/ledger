import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', 'build', 'libs', 'python')) 
import fetch

g = fetch.ml.Graph()
g.AddInput("Input")
g.AddFullyConnected("FC1", "Input", 28*28, 100)
g.AddRelu("Relu1", "FC1")
g.AddFullyConnected("FC2", "Relu1", 100, 100)
g.AddRelu("Relu2", "FC2")
g.AddFullyConnected("FC3", "Relu2", 100, 10)

t = fetch.math.tensor.Tensor([28, 28])
g.SetInput("Input", t)
res = g.Evaluate("FC3")

print(res.ToString())
