Fetch Machine Learning Library
==============================

Introduction
------------
This document gives an overview of the Fetch Machine Learning Library for developers.

The machine learning library is intended to be a generalised toolkit for building and running machine learning
applications on Fetch Network nodes, and that offers implementations in the Fetch VM language: Etch. Like PyTorch, and
TensorFlow there is a particular focus on neural networks and deep learning. Unlike these machine learning frameworks,
the Fetch machine learning library is compatible with the Fetch FixedPoint data type (and Tensors thereof). Since the
FixedPoint data type implements integer mathematics, It is possible to compute the precise result of complex
mathematical operations (which deep neural networks are) on different architectures with guaranteed identical results.
Since IEEE 754 (standard for floating-point arithmetic) does not guarantee that the same program will deliver identical
results on all conforming system; and since the c++ standard doesn't enforce IEEE 754, use of floats and doubles on
different architectures is almost always going to result in tiny differences at extremely high levels of precision.
For many applications this is no problem; however since the Fetch Network will be capable of computing and storing
the results of complex mathematical operations in smart contracts, these will need to be hashed and those hashes will
need to be absolutely identical on every architecture that attempts to verify them. This is not possible with existing
machine learning frameworks.


Machine Learning Library architecture
-------------------------
The core of the machine learning library is graph.hpp. This class represents a computation graph to which operation
nodes may be added and executed. It also manages setting inputs to nodes, saving and loading existing graph states
(the state_dict), as well as the uniqueness of variable names. node.hpp defines the NodeInterface for the nodes
stored on this graph.

The nodes on the graph represent operations to apply to Tensors. The graph may also contain special placeholder
operations which simply hold the place that a tensor will later fill when assigned in the training loop. A wide variety
of operations (including loss functions) are listed under ops; and layers are additional constructs that are useful for
wrapping up multiple ops that are commonly repeated (e.g. such as fully connected layer, which wraps the dot product
between the weights and the input tensor, followed by the addition of the bias tensor, and possibly also followed by an
activation function).

Finally the dataloaders are utilities for managing input training data when training the weights in the neural network.

The following block diagram gives a rough indication of the library structure (work in progress).

.. raw:: html
    :file: ml_lib_overview.svg


Important Notes for Working with the Machine Learning Library
-------------------------

- Every Op must have a DESCRIPTOR (this is for logging/error reporting)
- Node names are not guaranteed to be identical to the input string specified by the developer on AddNode (this is because the graph automatically resolves name collisions such that every node is uniquely named).
- Function ComputeOutputShape for classes that inherits from Ops is usually expensive and should be used only for initialization or in ASSERT. On Forward you can use output.shape() instead and for Backward there is error_signal.shape()
