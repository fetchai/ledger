#<cldoc:Developers Guide::Understanding the networking framework>

# Understanding the RPC framework
The RPC framework in the Fetch ledger separates into four components:
The application, the service delivery, the protocol and the service
implementation. As an example, we can think of a service build on top of
the TCP protocol in which case we would name the application, the
client, the service would be the TCP server, the protocol defines how we
pack the data and the implementation are the details on how the service
actually works: 

{{ :rpc_protocol.png?nolink&400 |}}

The separation of these four components makes it easy to change design
of part of the system without the need to rewrite the full stack every
time. For instance, if after writing a client-server applicatoin using
TCP, one would decide to use UDP instead, two of the components remains
intact and only the two top components would need rewriting. Likewise,
if one would decide to alter the implementation of a service only the
lower component would need changing. Finally, should one chose to make
the service available over multiple channels, say both TCP and UDP, this
can be achieved simply by adding another service layer to the stack. 

