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
from threading import Event

from fetch.cluster.instance import ConstellationInstance

from fetchai.ledger.api import TokenApi, TransactionApi
from fetchai.ledger.crypto import Identity

def output(*args):
    text = ' '.join(map(str, args))
    if text != '':
        sys.stdout.write(text)
        sys.stdout.write('\n')
        sys.stdout.flush()


class TimerWatchdog():
    """
    TimerWatchdog allows the user to specify a callback that will
    be executed after a set amount of time, unless the watchdog
    is stopped. This lets you dictate the length of tests.
    """
    def __init__(self, time, name, task , callback):

        self._time       = time
        self._name       = name
        self._task       = task
        self._callback   = callback
        self._stop_event = Event()
        self._stop_event.clear()

    def start(self):
        self._thread = threading.Thread(target=self._sleep)
        self._thread.start()

    def _sleep(self):
        # This will return false iff the stop event isn't set before the timeout
        if not self._stop_event.wait(self._time):
            output("Watchdog {} awoke before being stopped. Time: {} . Task: {}".format(self._name, self._time, self._task))
            self.trigger()
        else:
            output("Safely notified")

    # Notify the waiting thread - this causes it not to trigger.
    def stop(self):
        self._stop_event.set()

    def trigger(self):
        self._callback()

    def __del__(self):
        self.stop()

class TestInstance():
    """
    Sets up an instance of a test, containing references to started nodes and other relevant data
    """

    def __init__(self, build_directory, constellation_exe):

        self._number_of_nodes  = 0
        self._node_connections = None
        self._nodes_are_mining = []
        self._port_start_range = 8000
        self._port_range       = 20
        self._monitor          = None
        self._nodes            = []
        self._workspace        = ""
        self._node_workspace   = ""
        self._lanes            = 1
        self._slices           = 16
        self._max_test_time    = 1000
        self._nodes            = None
        self._metadata         = None
        self._watchdog         = None

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

        time.sleep(2) # TODO(HUT): blocking http call to node for ready state

    def stop(self):
        if self._nodes:
            for n, node in enumerate(self._nodes):
                print('Stopping Node {}...'.format(n))
                node.stop()
                print('Stopping Node {}...complete'.format(n))

        if self._watchdog:
            self._watchdog.stop()

def extract(test, key, expected = True, expect_type = None, default = None):
    """
    Convenience function to remove an item from a YAML string, specifying the type you expect to find
    """
    if key in test:
        result = test[key]

        if expect_type is not None and not isinstance(result, expect_type):
            output("Failed to get expected type from YAML! Key: {} YAML: {}".format(key, test))
            output("Note: expected type: {} got: {}".format(expect_type, type(result)))
            sys.exit(1)

        return result
    else:
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

    # Watchdog will trigger this if the tests exceeds allowed bounds. Note stopping the test cleanly is
    # necessary to preserve output logs etc.
    def clean_shutdown():
        output("Shutting down test due to failure!. Debug YAML: {}".format(test_yaml))
        test_instance.stop()
        os._exit(1)

    watchdog = TimerWatchdog(time = max_test_time, name = test_name, task = "End test and cleanup", callback = clean_shutdown)
    watchdog.start()

    test_instance._watchdog = watchdog

    # This shouldn't take a long time since nodes are started asynchronously
    test_instance.run()

def send_txs(parameters, test_instance):

    name   = parameters["name"]
    amount = parameters["amount"]
    nodes  = parameters["nodes"]

    if len(nodes) != 1:
        output("Only one node supported for sending TXs to at this time!")
        sys.exit(1)

    for node_index in nodes:
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

        # Verify TXs - will block until they have executed
        for tx, identity, balance in tx_and_identity:

            # Check TX has executed
            while True:
                status = txs.status(tx)

                if status == "Executed":
                    break
                time.sleep(0.1)

            seen_balance = tokens.balance(identity.public_key)
            if balance != seen_balance:
                output("Balance mismatch found after sending to node. Found {} expected {}".format(seen_balance, balance))
                test_instance._watchdog.trigger()

        output("Verified balances for node: {}".format(node_index))

def run_steps(test_yaml, test_instance):
    output("Running step: {}".format(test_yaml))

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

#TODO(HUT): just put this in steps as per Ed's suggestion
def verify_expectation(test_yaml, test_instance):
    output("Verifying test expectation: {}".format(test_yaml))

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

    # Read YAML file
    with open(yaml_file, 'r') as stream:
        try:
            all_yaml = yaml.load_all(stream)

            # Parse yaml documents as tests (sequentially)
            for test in all_yaml:
                # Create a new test instance
                output("\nTest: {}".format(extract(test, 'test_description')))

                # Create a test instance
                test_instance = TestInstance(build_directory, constellation_exe)

                # Configure the test - this will start the nodes asynchronously
                setup_test(extract(test, 'setup_conditions'), test_instance)

                # Run the steps in the test
                run_steps(extract(test, 'steps'), test_instance)

                # Verify expectations are met, such as being able to see certain balances
                verify_expectation(extract(test, 'expectation'), test_instance)

                test_instance.stop()
        except Exception as e:
            print('Failed to parse yaml or to run test! Error: "{}"'.format(str(e)))
            test_instance.stop()
            sys.exit(1)

    output("\nAll integration tests have passed :)")

def parse_commandline():
    parser = argparse.ArgumentParser( description='High level integration tests reads a yaml file, and runs the tests within. Returns 1 if failed')

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
