Fetch ledger architechture
==========================

Introduction
------------
This document gives a overview of the Fetch ledger architecture for developers.

The text is clickable.

.. This svg has onclick etc. which allows stuff
.. raw:: html
    :file: overview1.svg


Constellation application
-------------------------
The constellation application is one of many possible manifestations of a Fetch ledger node. It combines all the individual components into a single executable that is ready to connect to the rest of the network. The application is a commandline executable that can be used to start a node and start interacting with the network. The interaction can be carried out through the application's HTTP interface.

In order to get started with the constellation application, go to the Fetch source directory and type following:

.. code::

		mkdir build
		cd build
		cmake ..
		make constellation

This will compile the constellation binary and allow you to get started with the Fetch ledger. 

Generally, a Fetch node is comprised of a multitude of services that can be spread across computing units to archive the necessary scalability. A service is a server that provides a number of RPC protocol and each RPC protocol provides a number of functions associated with it. For more details on this, read the page on :ref:`service-composition`.

In the following section, we describe the various services inside the constellation application in more detail. In a nutshell, a Fetch ledger node is comprised of two front facing units: The ``P2PService`` and the ``Wallet Interface``. These are interacting with the underlying data stores and execution units as will be described in more detials in the coming sections. Both units interacts with the ``StorageUnitController`` and the ``Mainchain``. We summarise overview in the figure below:

.. mermaid::

	graph TD
		A[P2P Service] --> B[Mainchain Remote]
		A --> C[Storage Unit Controller]
		D[Wallet API] --> B
		D --> C
		C --> E[Lane 1]	
		C --> F[Lane 2]		
		C --> G[Lane N]		
		X[Execution Manager] --> B
		X --> C
		X --> Y1[Executor 1]
		X --> Y2[Executor 2]
		X --> YM[Executor M]		

The ``StorageUnitRemote`` serves two purposes: the first is to act as a direct interface to the storage components inside the lanes and the second is to receive feedback from these lanes about other Fetch nodes' behaviour in order to update a node trust model that is managed inside the ``P2PService``. This will be described in more details in the sections below.


Wallet API
----------
(Yet to be written)

P2P Service
-----------
The ``P2PService`` fulfills two purposes: firstly, it discovers other Fetch nodes and promotes their details to the lanes and mainchain in order for these to connect to them and perform synchronisation tasks. The second purpose is to keep track of which peers to trust and which to not trust. This is done continuously receiving feedback from the lanes and from the mainchain about the other nodes' behaviour and using that feedback to compute an overall trust score that is based on machine learning to predict what potential bad behaviour looks like.

The ``P2PService`` is built using a number of RPC interfaces. The first interface is responsible for ``IDENTITY`` exchange and allows another ``P2PService`` instances to ``PING``, say ``HELLO``, ``UPDATE_DETAILS``, ``EXCHANGE_ADDRESS`` and ``AUTHENTICATE`` itself. The ``PING`` function is used to test whether a peer is still alive. The function ``HELLO`` constitutes a mutual exchange of identities which are stored in the class ``PeerDetails``. Any call to ``HELLO`` expects its first argument to be a ``PeerDetails`` class and will always return a ``PeerDetails`` instance as well. This class contains information about the ``P2PService`` instance's public key, what the addresses of the mainchain and its lanes are. The function ``UPDATE_DETAILS`` can be used by a ``P2PService`` to inform its peers about changes to its configuration such as newly added lanes or changes of IP addresses. With ``EXCHANGE_ADDRESS`` a connecting peer can request its *external* (or as seen by the other peer) IP address. This functionality is used for a ``P2PService`` instance to maintain accurate details of its identity including endpoints. 



Mainchain
---------
Yet to be written

Storage unit
------------
Yet to be written

Lanes
+++++
