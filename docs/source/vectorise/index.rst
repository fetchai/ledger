Fetch Vectorisation Library
===========================
The Fetch vectorisation library depends on fetch-core to provide the
basic datastructures. You can check fetch-core out by initiating the
submodules of the supplied repository.

Objective
---------
The objective of the Fetch vectorisation library is to make it easy to
vectorise most common types of tasks with a minimal effort. An import
aspect is abstracting the underlying architecture away to archive
cross-platform compatibility.

Basic usage requirement
-----------------------
The vectorisation library is standalone and does as such not depend on
any modules. However, in order to compile the examples, fetch-core is
required. To initialise the submodules do

.. code-block:: bash

                git submodule init
                git submodule update

In order for the vectorisation module to work correctly it is important
that the memory is aligned according to the architecture
requirements. The Array<T> and the SharedArray<T> from fetch-core
takes care that this requirement is fulfilled. It is, however, possible
use fetch-vectorise with customly allocated memory. In this case it is
the developers responsibility to align the memory correctly.




High-level examples
-------------------
In the following sections we will take you through small examples
illustrating how the fetch- module can be applied in
different context to archive different goals. We will focus on
high-level examples. In the next section we will discuss more advanced
use cases where iteration over elements are done manually.

The fetch- module makes use of lambda functions, Functors
or function pointers to archive vectorisation. We refer to these
functions as *kernels*. Depending on which type of kernels are used,
performance will differ. C++11 introduced lambda functions which is a
great addition to the C++ language as it makes easy for the compiler to
inline functions. We therefore generally encourage the usage of lambda
functions as kernels. Likewise, Functors have been the traditional way
to make custom templated functionality and have been optimised over the
past three decades. For this reason, we also consider Functors are also
considered good pratice for vectorisation. We generally discourage
function pointers as these are much harder for the compiler to optimise
and as a result you may encounter a performance penalty.

The examples discussed can all be found in the examples directory of
fetch-vectorisation.

Elementwise manipulation
------------------------
In this example we will consider the ParallelDispatcher and its
memberfunction Apply(kernel, arg1, ... )
function that does elementwise manipulation to an array with an
arbitrary number of arguments.
We consider the case where we have two arrays A
and B. We now want compute the value of the elements in an array C,
through the formula:

.. math::
   C_{i} = \frac{1}{2} \frac{A_{i} - B_{i}}{ A_{i} + B_{i} }

We first investigate a standard C++ implementation. The source can be
found in examples/01_elementwise_manipulation/ordinary_solution.cpp
and we outline the essential part of the code below:

.. literalinclude:: ../../../libs/vectorise/examples/01_elementwise_manipulation/ordinary_solution.cpp
   :language: c++
   :lines: 7-18

The Fetch way of implementing the same function differs slightly. We an
inlined function to build a concurrent program. For the sake of making
the programming experience effortless, the vector_register_types
needed to do this are defined as typedefs in the fetch-core array data
structures. The example can be found in
examples/01_elementwise_manipulation/fetch_solution.cpp and core part is

.. literalinclude:: ../../../libs/vectorise/examples/01_elementwise_manipulation_fetch/main.cpp
   :language: c++
   :lines: 7-20

We note that this is some what different from the standard C++ code in
that it uses the inlined kernel to ensure that operations gets
vectorised. The kernel can be a kernel with an arbitrary amount of
arguments. Each of the arguments are added at the end of the Apply
function after the kernel argument. The kernel itself, follows the
signature kernel(arg1, ..., argN, ret) where arg1 through argN are
constant references and ret is a reference to an element in the
resulting array (C in the code above).

After building the example codes, the can be ran as follows:

.. code-block:: bash

                $ ./examples/element_wise_manip_ordinary 1000000
                6.08465 s
                $ ./examples/element_wise_manip_fetch 1000000
                6.06036 s

The difference in performance is hardly noticable. The reason for this
is that modern compilers are very good at optimising the instructions
according to the architecture.


Reduction
---------
Another common activity is simple reductions. The ParallelDispatcher
makes it easy to perform these as well. In this example we will consider
the reduction of a single array A through

.. math::
   r = \sum_{i=1}^N A_{i}

Below we outline the standard C++ implementation found in 02_reduction/ordinary_solution.cpp:

.. literalinclude:: ../../../libs/vectorise/examples/02_reduction/ordinary_solution.cpp
   :language: c++
   :lines: 10-19

Reimplementing this the Fetch way, we make use of the parallel
dispatchers Reduce function. The purpose of the reduce function is to
define how any to elements are reduced. It is in particular useful for
trivial functions such as addition, differences, any and all. We outline
the Fetch implementation here:

.. literalinclude:: ../../../libs/vectorise/examples/02_reduction_fetch/main.cpp
   :language: c++
   :lines: 12-22

This implementation can be found in
examples/02_reduction/fetch_solution.cpp.

.. code-block:: bash

                $ ./examples/reduction_ordinary 1000000000
                Preparing
                Computing
                1.05876 s to get 10.5083
                $ ./examples/reduction_fetch 1000000000
                Preparing
                Computing
                0.330216 s to get 10.5083

Again, we see that we get a bit more than a factor of three in
performance increase without much extra coding effort.

Sum Reduce
----------
The Fetch vectorisation library also supports more advanced reductions
such as the inner product between two vectors A and B. Consider
implementing the following functionality:

.. math::
   r = \sum_{i=1}^N (A_{i}-B_{i})^2

The standard C++ implementation is again quite trivial and can be found
in examples/03_sum_reduce/ordinary_solution.cpp.

.. literalinclude:: ../../../libs/vectorise/examples/03_sum_reduce/ordinary_solution.cpp
   :language: c++
   :lines: 7-20

In this implementation, we've chosen to use double rather than float
as the test input in the example would cause precision error with
floats. Writing the corresponding code is equvialent to writing the code
for floats - that is, due to the vector_register_type types the user
does not need to worry about the underlying data structures or assmbly
codes to perform the vectorisation:

.. literalinclude:: ../../../libs/vectorise/examples/03_sum_reduce_fetch/main.cpp
   :language: c++
   :lines: 8-24

This implementation can be found in
examples/03_sum_reduce/fetch_solution.cpp. Running a benchmark we get
following results with SSE enabled:

.. code-block:: bash

                $ ./examples/sum_reduce_ordinary 100000
                0.890618 s to get 0.100001
                $ ./examples/sum_reduce_fetch 100000
                0.44335 s to get 0.100001

In this case we get exactly the factor of two we would expect by
vectorising a double precision implementation.

Approximating Soft Max
----------------------
In this example we go through one of the more advanced use cases of the
vectorisation library, namely computing Soft Max function. We will
assume that we have an array A and that we want compute a the values
in a new array B according to:

.. math::
   B_i = \frac{ e^{A_i} }{ \sum_{j} e^{A_j} }

In order to do this, we will make use of an builtin approximation to the
exponential function. As a reference we first make an implementation
using the C++ standard library. The code can be found in  05_softmax_approx/ordinary_solution.cpp:

.. literalinclude:: ../../../libs/vectorise/examples/05_softmax_approx/ordinary_solution.cpp
   :language: c++
   :lines: 10-24


The corresponding Fetch implementation is listed below. Two functions
are noticiable in the implementation: approx_exp and reduce. The
first function approximates the exponential with up to 6% deviation from
the standard exponential and the second function reduces a vector
registers content to that of the underlying type. The algoithm is
implemented as follows:

.. literalinclude:: ../../../libs/vectorise/examples/05_softmax_approx_fetch/main.cpp
   :language: c++
   :lines: 13-31

A final thing to note is how we use the lambda functions capture list to
compute the as the kernel is applied. Notebly, the code is not more
complex than the intutitve standard implementation, but does it work better?

.. code-block:: bash

                $ ./examples/softmax_approx_ordinary 100000
                6.15118 s giving 7.89726e-06 for first element
                $ ./examples/softmax_approx_fetch 100000
                0.500866 s giving 7.56839e-06 for first element

This is an example where Fetch shines. The developer gains a factor of
more than a factor of ten at literally no extra cost in terms of code
complexity. This factor comes about as a result of the combination of a
well-designed exponential approximation together with concurrent
execution.

Ranges: Operating on a subset
-----------------------------
In many cases we do not want to operate on the full range of a shared array, but manipulate a subset of sequently occuring elements. To this end, the Fetch vectorisation framework introduces ranges which specify starting and ending elements for a vectorised operation.


.. todo::

        Write the rest of the example

Slicing: Operating on shifted arrays
------------------------------------
In a lot of cases, we do not want to manipulate arrays elementwise at the same indices. This is for instance the case when
caclulating the dot product of a matrix :math:`A` with the transposed of another matrix :math:`B` which leads to elements calculated as

.. math::
    \left[ A\cdot B^T \right]_{i,j} = \sum_{k=0}^{N-1} A(i, k)\cdot B(j, k) .

In this case we would vectorise over the index :math:`k`. We note that size of :math:`A` and :math:`B` may be different and therefore the meaning of :math:`k` is relative to some starting position that differs if :math:`A` and :math:`B` are represented internally as continous arrays.

To make it possible to implement routines like the one above we introduce the notation of slicing which represent a subview of a shared array with the constratint that starting position much be a multitude of the SIMD size. Any vectorisation method must use slices of same size.


Advanced use-cases
------------------
(yet to be written)

Vector iterators
++++++++++++++++
(yet to be written)

Dot product
+++++++++++
We now consider the case of more non-trivial use of the vectorisation
units. We will in this small example consider how to manually iterate
over an array and use vector operations to compute the reduction of a

.. literalinclude:: ../../../libs/vectorise/examples/08_dot_product/main.cpp
   :language: c++
   :lines: 0-5
