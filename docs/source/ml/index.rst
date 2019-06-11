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

The following block diagram shows the inheritance structure for some of the key components of the ML library (work in progress).

.. raw:: html
    :file: ml_inheritance.svg

The headings show the core concepts into which the library is divided, and the the blocks boredered with X's indicate abstract classes. Here we discuss further some of the core components:

Graph represents a computation graph containing nodes. When adding nodes to a graph the inputs to those nodes must be
specified, and must not result in a cycle; thus the graph must in fact be be a directed acyclic graph (DAG). Graph
also manages saving and loading existing graph states (a state dictionary), as checking for collisions in node names.
Nodes are merely the positions in the graph which know from which other nodes to draw inputs, and are able to perform
forward and backward computation according to a particular operation (Op). Each node is associated with a particular Op
and this defines the specifics of what happens whenever a the forward or backward methods are called. Often the forward
method will merely call upon an existing math function from the math library; for example the Add op called upon the
math library implementation for Add. The backward method will compute the gradient associated with that forward
operation given a particular error signal.

To inject data into the computation graph, special Ops are required that can have data assigned to them; these are the
placeholders. Weights are a specialisation of placeholders in that the data assigned to them is randomly initialised
and their data is trainable via back-propagation. Finally there may be further specialisations of weights, such as
embeddings, that offer additional functionality with respect to accessing and updating the data.

Layers, which inherit from SubGraphs, allow for wrapping up multiple Ops that are commonly repeated (e.g. such as fully
connected layer, which wraps the dot product between the weights and the input tensor, followed by the addition of the
bias tensor, and possibly also followed by an activation function).

Last of all, not shown on the diagram, are DataLoaders. These are for managing input training data when training the
weights in the neural network. They are responsible for parsing data into a format compatible with the rest of the
machine learning library, and then sampling data for training and testing. A specific dataloader is often needed for
each machine learning problem, as input data formats will tend to vary; however the DataLoader abstract class mandates
the necessary methods for all dataloaders.



General Notes for Working with the Machine Learning Library
-------------------------

- Every Op must have a DESCRIPTOR, this is used to generate a unique node name when name collisions are detected in a graph, as well as for logging/error reporting
- Node names are not guaranteed to be identical to the input string specified by the developer on AddNode (this is because the graph automatically resolves name collisions such that every node is uniquely named).
- Function ComputeOutputShape for classes that inherits from Ops is usually expensive and should be used only for initialization or in ASSERT. On Forward you can use output.shape() instead and for Backward there is error_signal.shape()
- Batch dimension is always trailing dimension, if you work with single datapoint it needs to have trailing dimension of size 1.
- Dataloaders are designed to reteturn pair of {Label,{Data}}, where Data are in vector.

