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
g.AddSoftmax("Softmax", "FC3")

criterion = fetch.ml.MeanSquareError()

gt = fetch.math.tensor.Tensor([10])
gt.Set([4], 1) # Overfitting the network to always output 1 in  pos 4
for i in range(0, 1000):
    t = fetch.math.tensor.Tensor([28, 28])
    g.SetInput("Input", t)
    res = g.Evaluate("Softmax")
    
    loss = criterion.Forward([res, gt])
    grad = criterion.Backward([res, gt])

    g.Backpropagate("Softmax", grad)
    g.Step(0.01)

    print(str(loss))
