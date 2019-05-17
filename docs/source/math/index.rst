Fetch Math Library
==========================

Introduction
------------
This document gives an overview of the math library for developers.

The math library is a header only fully-templated library. For this reason, the developer will need to be comfortable with SFINAE to understand much of the functionality.


Math Library architecture
-------------------------
A core component of the math library is the tensor.hpp class. This class is used to handle N-dimensional array mathematics.
This is crucial for the machine learning library, but can also be used more generally for any matrix algebra. Most of the rest
of the library is templated free functions that can be called with built-in types (double, int etc.), Tensors of built-in
types (Tensor<double>), Fetch types (core::FixedPoint<32, 32>), and Tensors of Fetch Types (Tensor<core::FixedPoint<32, 32>>).

fundamental_operations.hpp & matrix_operations.hpp contain most commonly used operations such as Add, Subtract, Multiply, &
Divide (in the former), and Max, ArgMax, Product, Sum, Dot etc. (in the latter). Additional standard operations are found
under standard_functions.

The following block diagram gives a rough indication of the library structure (work in progress).

.. raw:: html
    :file: math_lib_overview.svg


Tensor
-------------------------
Tensors are ultimately wrappers for the data_ object which is by default a SharedArray managed in the vectorise library.
Therein vectorisation/SIMD is managed on the underlying data. The role of the Tensor object, therefore, is to provide
interfaces for manipulating arrays at a level that makes sense in terms of maths/algebra, while allowing for actual
implementations to be efficient/vectorisable.

Tensors have related helper classes such as TensorIterator, TensorBroacast, & TensorSlice that permit efficient but
convenient manipulation such as accessing, transposing, and slicing.


Important Notes for Working with the Math Library
-------------------------

- Functions should usually be given two interface: one that takes a reference to the return object, and one that creates the return object internally
- Function design decisions should usually follow Numpy conventions where similar functionality exists