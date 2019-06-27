Getting started with the Fetch VM
=================================
Fetch are deploying virtual machime (VM) programs in a number of places including inside
agents, miners and smart contracts. To this end, we've developed a VM library to that is
aimed towards compute heavy tasks.

The objective of this tutorial is to take developers through the basic langauge of the VM and
how to extend the VM in C++. In order to get started with this tutorial you will need to
compile the `vm` examples and be familiar with the build system.


Writing your first VM code
--------------------------
In this first example we are going to build a `Hello world`-application. We will do that
using the first example vm code `example-vm-01-basic-vm`. To build this code type


.. code-block:: bash

                $ make example-vm-01-basic-vm

after having initialized the `build` directory. This will give you an executable
`./libs/vm/examples/example-vm-01-basic-vm` that can execute scripts. For now, we will not be
concerned with the the internals of this program but just get started with the language. The first
program we will write looks like this:

.. literalinclude:: ../../../libs/vm/examples/01_basic_vm/scripts/hello_world.fetch

You can try this script by running

.. code-block:: bash

                $ ./libs/vm/examples/example-vm-01-basic-vm ../libs/vm/examples/01_basic_vm/scripts/hello_world.fetch
                Hello world!

The Fetch language is strongly typed meaning that the type is determined at compile time. In many cases, types are
implicitly deduced but strongly enforced. Variable declaration is started with the keyword `var`. We give an example
of how one can declare variables is given in the example below:

.. literalinclude:: ../../../libs/vm/examples/01_basic_vm/scripts/hello_world2.fetch

In the above example we first declare a variable `hello` where the type is implicitly deduced as `String`. Then we
follow with an explicit declaration of type `String`. This variable is then set in the following lines. The result of
running this script is:

.. code-block:: bash

                $ ./libs/vm/examples/example-vm-01-basic-vm ../libs/vm/examples/01_basic_vm/scripts/hello_world2.fetch
                Hello world
                Hello Fetch

Finally, we discuss type-casting: The Fetch VM has no support for implicit casting by design. The reason for this
decision is that implicit casts has been a source of errors throughout history and we consider it bad practice.
Rather, one needs to use explicit casting. Casting operations are typically done using function `toTypeName`. In
the case of casting an `Int32` to a `String` you would simply use `toString`:

.. literalinclude:: ../../../libs/vm/examples/01_basic_vm/scripts/calculate.fetch

The result of executing this code a message stating `The result is: 5` as you would expect.


Branching and looping
---------------------
Next, let's look at loops and branches. While the VM does not support system arguments natively, `example-vm-01-basic-vm`
implements support for system arguments. Making loops in the Fetch VM is easy and below we show how one can iterate
over all passed system arguments:

.. literalinclude:: ../../../libs/vm/examples/01_basic_vm/scripts/using_system_args.fetch

This will, as you might expect, loop over the arguments appended to the execution command and print them one by one.
The Fetch VM of course also supports standard branching using `if`, `elseif`, `else` and `endif`. We show an example of
this below:

.. literalinclude:: ../../../libs/vm/examples/01_basic_vm/scripts/branching.fetch

Finally, the Fetch VM implements `while`, `endwhile` and also has support for `continue` and `break` for both for and
while loops.

Defining functions
------------------
The final basic topic we will touch upon in the language itself is the definition of functions. Like the rest of the language
functions are typed in both arguments and return type. In the example below we compute the Fibonacci numbers using recursion:

.. literalinclude:: ../../../libs/vm/examples/01_basic_vm/scripts/defining_functions.fetch

With this, the most basic functionality is covered and we will explore how to extend the VM in the next section.


Extending the functionality
===========================
In this section we will go through the details on how to build extensions for the VM. We will start out by
investigating the implementation of the `example-vm-01-basic-vm`.

Adding free functions
---------------------
In this first example we will consider how to implement the `print` function and the casting operation `toString` that we
used in the previous sections. The C++ implementations are given as

.. literalinclude:: ../../../libs/vm/examples/01_basic_vm/main.cpp
   :language: c++
   :lines: 28-36

New functionality can be added to the VM by defining a module and attach the functionality to it. We demonstrate
how this is done in the code below:

.. literalinclude:: ../../../libs/vm/examples/01_basic_vm/main.cpp
   :language: c++
   :lines: 78-80

The final bit consists of making the compiler and VM aware of the new functionality which is done by passing a pointer to
their constructors:

.. literalinclude:: ../../../libs/vm/examples/01_basic_vm/main.cpp
   :language: c++
   :lines: 87-88

After this, the new functionality is accessible from within the VM. You can find the full example in
`libs/vm/examples/01_basic_vm/main.cpp`.

Adding classes
--------------
The next topic we will consider is the introduction of new types.
(TODO: yet to be written)

Adding class constructors
-------------------------
(TODO: yet to be written)

Adding class member functions
-----------------------------
(TODO: yet to be written)

Adding static functions
-----------------------
In the previous section we used a class called ``System`` to access the supplied command line arguments. As the
VM does not have support for global objects in its current version, we here describe how this was implemented: We
created a class called ``System`` and attached two static functions to it. This was done as follows:

.. literalinclude:: ../../../libs/vm/examples/01_basic_vm/main.cpp
   :language: c++
   :lines: 39-54

with the function exposure implemented by

.. literalinclude:: ../../../libs/vm/examples/01_basic_vm/main.cpp
   :language: c++
   :lines: 82-84

You find the full implementation in `libs/vm/examples/01_basic_vm/main.cpp` and usage of this functionality is discussed
in the first section.


Building a smart contract language
==================================
In this section we will integrate the VM with the other modules in Fetch library to enable signature verification and
interact with the storage database. We follow the guide on creating static contracts :ref:`static-contracts-the-fetch-token`, but this time incorporate the VM with a custom module as described above to allow access to the ledgers ``StorageUnit``. The
header of the ``SmartContract`` implementation is as follows:

.. literalinclude:: ../../../libs/ledger/include/ledger/chaincode/smart_contract.hpp
   :language: c++
   :lines: 28-40

The smart contract has a single static contract function ``InvokeContract``. This function will be attached to all the
contract functions found in the supplied ``Script``. The contract is initialized with the script and the corresponding
``Module`` is create with the contract attached to it exposing the functionality to get access to the state database.
The exposure of the functions themselves to the ledger is done by making a loop that runs over the
functions defined in the script:

.. literalinclude:: ../../../libs/ledger/src/chaincode/smart_contract.cpp
   :language: c++
   :lines: 30-40

This attaches all the contract functions to the ledger and makes that the function ``SmartContract::InvokeContract`` is
called whenever a transaction is dispatched to this contract. The function ``CreateVMDefinition`` creates a module with
this specific contract instance attached to it, hence ensuring that all data access is done through ``this`` using the
corresponding ``StorageUnit``.

Finally, the code that actually invokes the smart contract is implemented in the ``InvokeContract`` function which
first extracts the name and then tries to execute the corresponding script function:

.. literalinclude:: ../../../libs/ledger/src/chaincode/smart_contract.cpp
   :language: c++
   :lines: 41-52

If the script returns failed, the contract invocation is marked as failed overall.
