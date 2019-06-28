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

.. code-block:: cpp
   :linenos:

   class TokenContract : public Contract
   {
   public:
     TokenContract();
     ~TokenContract() = default;

     static constexpr char const *LOGGING_NAME = "TokenContract";

   private:
     // transaction handlers
     Status CreateWealth(Transaction const &tx);
     Status Transfer(Transaction const &tx);

     // queries
     Status Balance(Query const &query, Query &response);
   };

The header file above clearly defines the contract calls and the queries that you can make.
The constructor of the contract subclass exposes these methods to make the ledger aware of their
existence. This is done as shown below:

.. code-block:: cpp
   :linenos:

   class TokenContract : public Contract
   {
   public:
     TokenContract();
     ~TokenContract() = default;

     static constexpr char const *LOGGING_NAME = "TokenContract";

   private:
     // transaction handlers
     Status CreateWealth(Transaction const &tx);
     Status Transfer(Transaction const &tx);

     // queries
     Status Balance(Query const &query, Query &response);
   };

The remaining bits of this contract consists of implementing the business logic governing the
contract functions above.

(TODO: Describe these)

Smart contracts
---------------
(TODO: to be written after the VM section on the implementation)
