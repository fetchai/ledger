import sys, os
if len(sys.argv) > 1:
    sys.path.append(sys.argv[1])
else:
    print("Usage : " + sys.argv[0] + " PATH_TO_fetch.cpython_*")
    print("Should be ../../../../build/libs/python/")
    sys.exit(0)

import fetch

import fetch.ml.fixed_32_32 as ml

output_layer = "Softmax"
learning_rate = 0.01
training_iteration = 1000

g = ml.Graph()
g.AddInput("Input")
g.AddFullyConnected("FC1", "Input", 28*28, 100)
g.AddRelu("Relu1", "FC1")
g.AddFullyConnected("FC2", "Relu1", 100, 100)
g.AddRelu("Relu2", "FC2")
g.AddFullyConnected("FC3", "Relu2", 100, 10)
g.AddSoftmax("Softmax", "FC3")

criterion = ml.MeanSquareError()

gt = fetch.math.tensor.TensorFixed32_32([10])
gt.Set([4], 1) # Overfitting the network to always output 1 in  pos 4
for i in range(0, training_iteration):
    t = fetch.math.tensor.TensorFixed32_32([28, 28])
    g.SetInput("Input", t)
    res = g.Evaluate("Softmax")
    
    loss = criterion.Forward([res, gt])
    grad = criterion.Backward([res, gt])

    g.Backpropagate(output_layer, grad)
    g.Step(learning_rate)

    print(str(loss))
