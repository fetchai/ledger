import fetch.ml.fixed_32_32 as ml
import fetch
import sys
import os
if len(sys.argv) > 1:
    sys.path.append(sys.argv[1])
else:
    print("Usage : " + sys.argv[0] + " PATH_TO_fetch.cpython_*")
    print("Should be ../../../../build/libs/python/")
    sys.exit(0)


output_layer = "Softmax"
learning_rate = 0.01
training_iteration = 1000

g = ml.Graph()
g.AddInput("Input")
g.AddFullyConnected("FC1", "Input", 28 * 28, 100)
g.AddRelu("Relu1", "FC1")
g.AddFullyConnected("FC2", "Relu1", 100, 100)
g.AddRelu("Relu2", "FC2")
g.AddFullyConnected("FC3", "Relu2", 100, 10)
g.AddSoftmax("Softmax", "FC3")

criterion = ml.MeanSquareError()
dataloader = ml.MNISTLoader(
    "/PATH/TO/train-images-idx3-ubyte", "/PATH/TO/train-labels-idx1-ubyte")

for i in range(0, training_iteration):
    sample = dataloader.GetNext()
    g.SetInput("Input", sample[1])
    res = g.Evaluate("Softmax")

    gt = fetch.math.tensor.TensorFixed32_32([1, 10])
    gt.Set([0, sample[0]], 1)

    loss = criterion.Forward([res, gt])
    grad = criterion.Backward([res, gt])

    g.Backpropagate(output_layer, grad)
    g.Step(learning_rate)

    print(str(loss))
