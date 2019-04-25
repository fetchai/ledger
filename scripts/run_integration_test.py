#!/usr/bin/env python3

#
# Integration tests of fetch-ledger
#
# This is achieved by using available fetch APIs to spin up a network locally
# and test it can handle certain conditions (such as single or multiple node failure)
#

import sys
import os
import argparse
import yaml
import io
import random
import datetime
import time
import threading
import glob
import shutil

from fetch.cluster.instance import ConstellationInstance
#from fetch.cluster.monitor import ConstellationMonitor

from fetchai.ledger.api import TokenApi, TransactionApi
from fetchai.ledger.crypto import Identity

def output(*args):
    text = ' '.join(map(str, args))
    if text != '':
        sys.stdout.write(text)
        sys.stdout.write('\n')
        sys.stdout.flush()

class StepsInfo():
    pass

class TimerWatchdog():

    def __init__(self, time, name, task = "unspecified", callback = None):

        self._time     = time
        self._name     = name
        self._task     = task
        self._callback = callback
        self._stopped  = False

    def start(self):
        self._thread = threading.Thread(target=self._sleep)
        self._thread.daemon = True
        self._thread.start()

    def _sleep(self):
        time.sleep(self._time)

        if self._stopped == False:
            output("Watchdog {} awoke before being stopped. Time: {} . Task: {}".format(self._name, self._time, self._task))

        self.trigger()

    def stop(self):
        self._stopped = True

    def trigger(self):
        if self._callback:
            self._callback()

class TestInstance():

    _number_of_nodes  = 0
    _node_connections = None
    _nodes_are_mining = []
    _port_start_range = 8000
    _port_range       = 20
    _monitor          = None
    _nodes            = []
    _workspace        = ""
    _node_workspace   = ""
    _lanes            = 1
    _slices           = 16
    _max_test_time    = 1000
    _nodes            = None
    _metadata         = None
    _watchdog         = None

    def __init__(self, build_directory, constellation_exe):
        print("Found build_dir: {}, const: {}".format(build_directory, constellation_exe));

        # Default to removing old tests
        for f in glob.glob(build_directory + "/integration_test_*"):
            shutil.rmtree(f)

        # To avoid possible collisions, prepend output files with the date
        self._random_identifer  = '{0:%Y_%m_%d_%H_%M_%S}'.format(datetime.datetime.now())
        self._workspace         = os.path.join(build_directory, 'integration_test_{}'.format(self._random_identifer))
        self._build_directory   = build_directory
        self._constellation_exe = os.path.abspath(constellation_exe)

        # Ensure that the constellation exe exists
        if not os.path.isfile(self._constellation_exe):
            output("Couldn't find supposed constellation exe: {}".format(constellation_exe))
            sys.exit(1)

        # Ensure that build/integration_output_XXX/ exists for the test output
        os.makedirs(self._workspace, exist_ok=True)

    def run(self):

        if(self._number_of_nodes == 0):
            output("Attempted to run integration test with 0 nodes - that's not right.")
            sys.exit(1)

        nodes = []

        # build up all the instances
        for index in range(self._number_of_nodes):

            # Create a folder for the node to write logs to etc.
            root = os.path.abspath(os.path.join(self._workspace, 'node{}'.format(index)))

            # ensure the workspace folder exits
            os.makedirs(root, exist_ok=True)

            port = self._port_start_range + (self._port_range * index)

            # Create an instance of the constellation
            instance = ConstellationInstance(
                self._constellation_exe,
                port,
                root
            )

            # configure the lanes and slices
            instance.lanes  = self._lanes
            instance.slices = self._slices

            nodes.append(instance)

        # Now connect the nodes as specified
        if self._node_connections:
            for connect_from, connect_to in self._node_connections:
                nodes[connect_from].add_peer(nodes[connect_to])

        # Enable mining node(s)
        for miner_index in self._nodes_are_mining:
            nodes[miner_index].mining = True

        # In the case only one miner node, it runs in standalone mode
        if(len(nodes) == 1 and len(self._nodes_are_mining) > 0):
            nodes[0].standalone = True

        # start all the nodes
        for n, node in enumerate(nodes):
            print('Starting Node {}...'.format(n))

            node.start()
            print('Starting Node {}...complete'.format(n))

            time.sleep(0.5)

        self._nodes = nodes

    def stop(self):
        # stop all the nodes
        for n, node in enumerate(self._nodes):
            print('Stopping Node {}...'.format(n))
            node.stop()
            print('Stopping Node {}...complete'.format(n))

        if self._watchdog:
            self._watchdog.stop()

def extract(test, key, expected = True, expect_type = None, default = None):
    try:
        result = test[key]

        if expect_type is not None and not isinstance(result, expect_type):
            output("Failed to get expected type from YAML! Key: {} YAML: {}".format(key, test))
            output("Note: expected type: {} got: {}".format(expect_type, type(result)))
            sys.exit(1)

        return result
    except:
        if expected:
            output("Failed to find key in YAML! \nKey: {} \nYAML: {}".format(key, test))
            sys.exit(1)
        else:
            return default

def setup_test(test_yaml, test_instance):
    output("Setting up test: {}".format(test_yaml))

    test_name        = extract(test_yaml, 'test_name', expected = True, expect_type = str)
    number_of_nodes  = extract(test_yaml, 'number_of_nodes', expected = True, expect_type = int)
    node_connections = extract(test_yaml, 'node_connections', expected = False, expect_type = list)
    mining_nodes     = extract(test_yaml, 'mining_nodes', expected = False, expect_type = list, default = [])
    max_test_time    = extract(test_yaml, 'max_test_time', expected = False, expect_type = int, default = 10)

    test_instance._number_of_nodes  = number_of_nodes
    test_instance._node_connections = node_connections
    test_instance._nodes_are_mining = mining_nodes
    test_instance._max_test_time    = max_test_time

    def clean_shutdown():
        output("Shutting down test due to failure!. YAML: {}".format(test_instance))
        test_instance.stop()
        os._exit(1) # REALLY ensure the program quits.

    # Running/setting up the test could take too long - auto-kill if this happens (needs to cleanup constellation threads)
    watchdog = TimerWatchdog(time = max_test_time, name = test_name, task = "End test and cleanup", callback = clean_shutdown)
    watchdog.start()

    test_instance._watchdog = watchdog

    # This shouldn't take a long time since nodes are async
    test_instance.run()

def send_txs(parameters, test_instance):

    name   = parameters["name"]
    amount = parameters["amount"]
    nodes  = parameters["nodes"]

    if len(nodes) != 1:
        output("Only one node supported for sending TXs to at this time!")
        sys.exit(1)

    # TODO(HUT): discuss best way to determine nodes are ready. Probably using http interface
    time.sleep(2)

    for node_index in nodes:

        # TODO(HUT): Refactor this as a getter, and ConstellationInstances to know their location
        node_host = "localhost"
        node_port = test_instance._nodes[node_index]._port_start

        # create the API objects we use to interface with the nodes
        txs = TransactionApi(node_host, node_port)
        tokens = TokenApi(node_host, node_port)

        tx_and_identity = []

        for balance in range(amount):

            # generate a random identity
            identity = Identity()

            # create and send the transaction to the ledger, capturing the tx hash
            tx = tokens.wealth(identity.private_key_bytes, balance)

            tx_and_identity.append((tx, identity, balance))

            output("Created wealth with balance: ", balance)

        # Conditionally verify TXs get executed
        # TODO(HUT): This won't work if the node isn't a miner, I suspect
        if False:
            for tx, identity, balance in tx_and_identity:

                # wait while we poll to see when this transaction has been completed
                prev_status = None
                while True:
                    status = txs.status(tx)

                    # print the changes in tx status
                    if status != prev_status:
                        print('Current Status:', status)
                        prev_status = status

                    # exit the wait loop once the transaction has been executed
                    if status == "Executed":
                        break

                # check the balance now
                while True:
                    seen_balance = tokens.balance(identity.public_key)
                    if balance != seen_balance:
                        output("Balance mismatch found after sending to node. Found {} expected {}".format(seen_balance, balance))
                    else:
                        break;
                    time.sleep(1)

        # Attach this to the test instance so it can be used for verification
        test_instance._metadata = tx_and_identity

def verify_txs(parameters, test_instance):

    name   = parameters["name"]
    nodes  = parameters["nodes"]

    # Currently assume there only one set of TXs
    tx_and_identity = test_instance._metadata

    for node_index in nodes:
        node_host = "localhost"
        node_port = test_instance._nodes[node_index]._port_start

        txs = TransactionApi(node_host, node_port)
        tokens = TokenApi(node_host, node_port)

        # Verify TXs - these must have been executed by now!
        for tx, identity, balance in tx_and_identity:

            seen_balance = tokens.balance(identity.public_key)
            if balance != seen_balance:
                output("Balance mismatch found after sending to node. Found {} expected {}".format(seen_balance, balance))
                test_instance._watchdog.trigger()

        output("Verified balances for node: {}".format(node_index))

def run_steps(test_yaml, test_instance):
    output("Running test: {}".format(test_yaml))

    for step in test_yaml:
        command    = list(step.keys())[0]
        parameters = step[command]

        if command == 'send_txs':
            send_txs(parameters, test_instance)
        elif command == 'sleep':
            time.sleep(parameters)
        else:
            output("Found unknown command when running steps: {}".format(command))
            sys.exit(1)

def verify_expectation(test_yaml, test_instance):
    output("Running test: {}".format(test_yaml))

    for step in test_yaml:
        command    = list(step.keys())[0]
        parameters = step[command]

        if command == 'verify_txs':
            print("verifying txs")
            verify_txs(parameters, test_instance)
        else:
            output("Found unknown command when verifying expectations: {}".format(command))
            sys.exit(1)

def run_test(build_directory, yaml_file, constellation_exe):

    data_loaded = []

    # Read YAML file
    with open(yaml_file, 'r') as stream:
        try:
            data_loaded = yaml.load_all(stream)

            # https://stackoverflow.com/questions/42802058
            data_loaded = list(data_loaded)
        except Exception as e:
            print('Failed to parse yaml!\n\n'+ str(e))

    # Parse yaml documents as tests (sequentially)
    for test in data_loaded:
        # Create a new test instance
        output("\nTest: {}".format(test))

        # Create a test instance
        test_instance = TestInstance(build_directory, constellation_exe)

        # Configure the test - this will start the nodes asynchronously
        setup_test(extract(test, 'setup_conditions'), test_instance)

        # Run the steps in the test
        run_steps(extract(test, 'steps'), test_instance)

        # Verify expectations are met, such as being able to see certain balances
        verify_expectation(extract(test, 'expectation'), test_instance)

        test_instance.stop()

    output("\nAll integration tests have passed :)")

def parse_commandline():
    parser = argparse.ArgumentParser( description='TODO(HUT): description')

    # Required argument
    parser.add_argument('build_directory', type=str, help='Location of the build directory relative to current path')
    parser.add_argument('yaml_file', type=str, help='Location of the build directory relative to current path')
    parser.add_argument('constellation_exe', type=str, help='Location of the build directory relative to current path')

    return parser.parse_args()

def main():
    args = parse_commandline()

    return run_test(args.build_directory, args.yaml_file, args.constellation_exe)

if __name__ == '__main__':
    main()



#if args.interconnect == 'chain':

#    # inter connect the nodes (single chain)
#    for n in range(len(nodes)):
#        for m in range(args.chain_length):
#            o = n - (m + 1)
#            if o >= 0:
#                nodes[n].add_peer(nodes[o])


#        elif args.interconnect == 'chaos':
#            all_nodes = set(list(range(len(nodes))))
#
#            connections_pairs = set()
#
#            for n in all_nodes:
#                possible_nodes = list()
#                for m in all_nodes - {n}:
#                    if (n, m) in connections_pairs or (m, n) in connections_pairs:
#                        continue
#                    possible_nodes.append(m)
#
#                if len(possible_nodes) == 0:
#                    raise RuntimeError('Chaos size too large to support this network, please choose a lower number')
#                elif len(possible_nodes) > args.chaos_size:
#                    random.shuffle(possible_nodes)
#                    possible_nodes = possible_nodes[:args.chaos_size]
#
#                for m in possible_nodes:
#                    connections_pairs.add((n, m))
#
#            # build a debug map and print it
#            connection_map = {}
#            for n, m in connections_pairs:
#                connections = connection_map.get(n, set())
#                connections.add(m)
#                connection_map[n] = connections
#            for n, ms in connection_map.items():
#                print('- {} -> {}'.format(n, ','.join(map(str, ms))))
#
#            # make the connections
#            for n, m in connections_pairs:
#                nodes[n].add_peer(nodes[m])
#
#        else:
#            assert False
