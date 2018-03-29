Using the OEF
========

This readme will serve as reference for using the OEF.

The current branch is feature/addOEF.

You should follow the toplevel README instructions to get the correct dependencies.

In addition to this, if you are running python you will need to pip install requests.


Using the OEF in single-node mode
========

A brief example program has been made. Build, run and inspect the following files, in order

./build/examples/oef_server
./build/examples/generate_random_aeas
./build/examples/query_random_aeas


It may be useful to inspect the HTTP debug interface that is attached and syncronised to all nodes:

http://localhost:8080/debug-all-agents
http://localhost:8080/debug-all-events
http://localhost:8080/debug-all-nodes
