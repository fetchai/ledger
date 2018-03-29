Using the OEF
========

This readme will serve as reference for using the OEF.

The current branch is feature/addOEF.

You should follow the toplevel README instructions to get the correct dependencies.

In addition to this, if you are running python you will need to pip install requests.


Using the OEF in single-node mode
========

A brief example program has been made. Build, run and inspect the following files, in order

- spin up an instance of an OEF node
./build/examples/oef_server

- Generate a number of random AEAs and connect them (you can see these on the http debug-all-agents page)
./build/examples/generate_random_aeas

- Query the OEF for some of these AEAs, and 'buy' from them
./build/examples/query_random_aeas

It may be useful to inspect the HTTP debug interface that is attached and syncronised to all nodes:

http://localhost:8080/debug-all-agents
http://localhost:8080/debug-all-events
http://localhost:8080/debug-all-nodes

Notes
========

Note that the only event which is currently logged is query events

There is no connection to the fake ledger by AEAs
