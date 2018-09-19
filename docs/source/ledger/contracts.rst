Static and dynamic contracts
============================
The Fetch ledger makes use of multiple types of ledger programs. The first and most fundamental
type of programs is static contracts. Static contracts are business logic that is built at the 
time when the ledger is compiled and the code is "unchangable" in the sense that the only way to update
the code or new contracts is recompile the ledger and distribute new executables to all participants. Smart contracts
is the second type business logic (dynamic contracts) found in the Fetch ledger. Smart contracts are 
programmable at run time and are smart peices of business logic running in the Fetch VM.


Contract execution
------------------
``Contracts`` are instantiated by ``Executors`` who run the contract. The execution itself is managed 
and orchestrated by the ``ExecutionManager``.


.. _static-contracts-the-fetch-token:

Static contracts: The Fetch token
---------------------------------
In order to implement a static contract, we need to create a new class that inherits from
the ``Contract`` class and expose the functions that we want available in the contract. In 
the following we will describe an implementation of the the Fetch token. In order to create
a new contract code, we define the body of the contract as below:

.. literalinclude:: ../../../libs/ledger/include/ledger/chaincode/token_contract.hpp
   :language: c++
   :lines: 25-38

The header file above clearly defines the contract calls and the queries that you can make.
The constructor of the contract subclass exposes these methods to make the ledger aware of their
existence. This is done as shown below:

.. literalinclude:: ../../../libs/ledger/src/chaincode/token_contract.cpp
   :language: c++
   :lines: 50-56

The remaining bits of this contract consists of implementing the business logic governing the
contract functions above.

(TODO: Describe these)

Smart contracts
---------------
(TODO: to be written after the VM section on the implementation)