#!/usr/bin/env python3

#
# end to end tests of fetch-ledger
#
# This is achieved by using available fetch APIs to spin up a network locally
# and test it can handle certain conditions (such as single or multiple
# node failure)

import sys
import os
import argparse
import yaml
import io
import random
import datetime
import importlib
import time
import threading
import glob
import shutil
import traceback
import time
import pickle
from threading import Event
from pathlib import Path

from fetch.cluster.instance import ConstellationInstance

from fetchai.ledger.api import LedgerApi
from fetchai.ledger.crypto import Entity


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

    def __init__(self, time, name, task, callback):

        self._time = time
        self._name = name
        self._task = task
        self._callback = callback
        self._stop_event = Event()
        self._stop_event.clear()

    def start(self):
        self._thread = threading.Thread(target=self._sleep)
        self._thread.start()

    def _sleep(self):
        # This will return false iff the stop event isn't set before the
        # timeout
        if not self._stop_event.wait(self._time):
            output(
                "Watchdog '{}' awoke before being stopped! Awoke after: {}s . Watchdog will now: {}".format(
                    self._name,
                    self._time,
                    self._task))
            self.trigger()
        else:
            output("Watchdog safely stopped")

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

    def __init__(self, build_directory, constellation_exe, yaml_file):

        self._number_of_nodes = 0
        self._node_load_directory = []
        self._node_connections = None
        self._nodes_are_mining = []
        self._port_start_range = 8000
        self._port_range = 20
        self._workspace = ""
        self._lanes = 1
        self._slices = 16
        self._max_test_time = 1000
        self._nodes = []
        self._metadata = None
        self._watchdog = None
        self._creation_time = time.perf_counter()

        # Default to removing old tests
        for f in glob.glob(build_directory + "/end_to_end_test_*"):
            shutil.rmtree(f)

        # To avoid possible collisions, prepend output files with the date
        self._random_identifer = '{0:%Y_%m_%d_%H_%M_%S}'.format(
            datetime.datetime.now())
        self._workspace = os.path.join(
            build_directory, 'end_to_end_test_{}'.format(
                self._random_identifer))
        self._build_directory = build_directory
        self._constellation_exe = os.path.abspath(constellation_exe)
        self._yaml_file = os.path.abspath(yaml_file)
        self._test_files_dir = os.path.dirname(self._yaml_file)

        verify_file(constellation_exe)
        verify_file(self._yaml_file)

        # Ensure that build/end_to_end_output_XXX/ exists for the test output
        os.makedirs(self._workspace, exist_ok=True)

    def append_node(self, index, load_directory=None):
        # Create a folder for the node to write logs to etc.
        root = os.path.abspath(os.path.join(
            self._workspace, 'node{}'.format(index)))

        # ensure the workspace folder exits
        os.makedirs(root, exist_ok=True)

        if load_directory and index in load_directory:
            load_from = self._test_files_dir + \
                "/nodes_saved/" + load_directory[index]
            files = os.listdir(load_from)

            for f in files:
                shutil.copy(load_from + f, root)

        port = self._port_start_range + (self._port_range * index)

        # Create an instance of the constellation - note we don't clear path since
        # it should be clear unless load_directory is used
        instance = ConstellationInstance(
            self._constellation_exe,
            port,
            root,
            clear_path=False
        )

        # configure the lanes and slices
        instance.lanes = self._lanes
        instance.slices = self._slices

        assert len(self._nodes) == index, "Attempt to add node with an index mismatch. Current len: {}, index: {}".format(
            len(self._nodes), index)

        self._nodes.append(instance)

    def connect_nodes(self, node_connections):
        for connect_from, connect_to in node_connections:
            self._nodes[connect_from].add_peer(self._nodes[connect_to])
            output("Connect node {} to {}".format(connect_from, connect_to))

    def start_node(self, index):
        print('Starting Node {}...'.format(index))

        self._nodes[index].start()
        print('Starting Node {}...complete'.format(index))

        time.sleep(0.5)

    def restart_node(self, index):
        print('Restarting Node {}...'.format(index))

        self._nodes[index].stop()

        self.dump_debug(index)

        pattern = ["*.db"]

        for p in pattern:
            [os.remove(x) for x in glob.iglob('./**/' + p, recursive=True)]

        self.start_node(index)

        time.sleep(3)

    def print_time_elapsed(self):
        output("Elapsed time: {}".format(
            time.perf_counter() - self._creation_time))

    def run(self):

        # build up all the node instances
        for index in range(self._number_of_nodes):
            output("Adding!")
            self.append_node(index, self._node_load_directory)

        output("Added!")
        # Now connect the nodes as specified
        if self._node_connections:
            self.connect_nodes(self._node_connections)

        # Enable mining node(s)
        for miner_index in self._nodes_are_mining:
            self._nodes[miner_index].mining = True

        # In the case only one miner node, it runs in standalone mode
        if(len(self._nodes) == 1 and len(self._nodes_are_mining) > 0):
            self._nodes[0].standalone = True
        else:
            for node in self._nodes:
                node.private_network = True

        # start all the nodes
        for index in range(self._number_of_nodes):
            self.start_node(index)

        time.sleep(2)  # TODO(HUT): blocking http call to node for ready state

    def stop(self):
        if self._nodes:
            for n, node in enumerate(self._nodes):
                print('Stopping Node {}...'.format(n))
                node.stop()
                print('Stopping Node {}...complete'.format(n))

        if self._watchdog:
            self._watchdog.stop()

    # If something goes wrong, print out debug state (mainly node log files)
    def dump_debug(self, only_node = None):
        if self._nodes:
            for n, node in enumerate(self._nodes):

                if only_node is not None and n is not only_node:
                    continue

                print('\nNode debug. Node:{}'.format(n))
                node_log_path = node.log_path

                if not os.path.isfile(node_log_path):
                    output("Couldn't find supposed node log file: {}".format(
                        node_log_path))
                else:
                    # Send raw bytes directly to stdout since it contains
                    # non-ascii
                    data = Path(node_log_path).read_bytes()
                    sys.stdout.buffer.write(data)
                    sys.stdout.flush()


def verify_file(filename):
    if not os.path.isfile(filename):
        output("Couldn't find expected file: {}".format(filename))
        sys.exit(1)


def extract(test, key, expected=True, expect_type=None, default=None):
    """
    Convenience function to remove an item from a YAML string, specifying the type you expect to find
    """
    if key in test:
        result = test[key]

        if expect_type is not None and not isinstance(result, expect_type):
            output(
                "Failed to get expected type from YAML! Key: {} YAML: {}".format(
                    key, test))
            output("Note: expected type: {} got: {}".format(
                expect_type, type(result)))
            sys.exit(1)

        return result
    else:
        if expected:
            output(
                "Failed to find key in YAML! \nKey: {} \nYAML: {}".format(
                    key, test))
            sys.exit(1)
        else:
            return default


def setup_test(test_yaml, test_instance):
    output("Setting up test: {}".format(test_yaml))

    test_name = extract(test_yaml, 'test_name', expected=True, expect_type=str)
    number_of_nodes = extract(
        test_yaml, 'number_of_nodes', expected=True, expect_type=int)
    node_load_directory = extract(
        test_yaml, 'node_load_directory', expected=False, expect_type=dict)
    node_connections = extract(
        test_yaml, 'node_connections', expected=False, expect_type=list)
    mining_nodes = extract(test_yaml, 'mining_nodes',
                           expected=False, expect_type=list, default=[])
    max_test_time = extract(test_yaml, 'max_test_time',
                            expected=False, expect_type=int, default=10)

    test_instance._number_of_nodes = number_of_nodes
    test_instance._node_load_directory = node_load_directory
    test_instance._node_connections = node_connections
    test_instance._nodes_are_mining = mining_nodes
    test_instance._max_test_time = max_test_time

    # Watchdog will trigger this if the tests exceeds allowed bounds. Note stopping the test cleanly is
    # necessary to preserve output logs etc.
    def clean_shutdown():
        output(
            "***** Shutting down test due to failure!. Debug YAML: {} *****\n".format(test_yaml))
        test_instance.stop()
        test_instance.dump_debug()
        os._exit(1)

    watchdog = TimerWatchdog(
        time=max_test_time,
        name=test_name,
        task="End test and cleanup",
        callback=clean_shutdown)
    watchdog.start()

    test_instance._watchdog = watchdog

    # This shouldn't take a long time since nodes are started asynchronously
    test_instance.run()


def send_txs(parameters, test_instance):

    name = parameters["name"]
    amount = parameters["amount"]
    nodes = parameters["nodes"]

    if len(nodes) != 1:
        output("Only one node supported for sending TXs to at this time!")
        sys.exit(1)

    # Create or load the identities up front
    identities = []

    if "load_from_file" in parameters and parameters["load_from_file"] == True:

        filename = "{}/identities_pickled/{}.pickle".format(
            test_instance._test_files_dir, name)

        verify_file(filename)

        with open(filename, 'rb') as handle:
            identities = pickle.load(handle)
    else:
        identities = [Entity() for i in range(amount)]

    # If pickling, save this to the workspace
    with open('{}/{}.pickle'.format(test_instance._workspace, name), 'wb') as handle:
        pickle.dump(identities, handle)

    for node_index in nodes:
        node_host = "localhost"
        node_port = test_instance._nodes[node_index]._port_start

        # create the API objects we use to interface with the nodes
        api = LedgerApi(node_host, node_port)

        tx_and_identity = []

        for index in range(amount):

            # get next identity
            identity = identities[index]

            # create and send the transaction to the ledger, capturing the tx
            # hash
            tx = api.tokens.wealth(identity, index)

            tx_and_identity.append((tx, identity, index))

            output("Created wealth with balance: ", index)

        # Attach this to the test instance so it can be used for verification
        test_instance._metadata = tx_and_identity

        # Save the metatada too
        with open('{}/{}_meta.pickle'.format(test_instance._workspace, name), 'wb') as handle:
            pickle.dump(test_instance._metadata, handle)


def run_python_test(parameters, test_instance):
    host = parameters.get('host', 'localhost')
    port = parameters.get('port', test_instance._nodes[0]._port_start)

    test_script = importlib.import_module(
        parameters['script'], 'end_to_end_test')
    test_script.run({
        'host': host,
        'port': port
    })


def verify_txs(parameters, test_instance):

    name = parameters["name"]
    nodes = parameters["nodes"]
    expect_mined = False

    try:
        expect_mined = parameters["expect_mined"]
    except:
        pass

    # Currently assume there only one set of TXs
    tx_and_identity = test_instance._metadata

    # Load these from file if specified
    if "load_from_file" in parameters and parameters["load_from_file"] == True:

        filename = "{}/identities_pickled/{}_meta.pickle".format(
            test_instance._test_files_dir, name)

        verify_file(filename)

        with open(filename, 'rb') as handle:
            tx_and_identity = pickle.load(handle)

    for node_index in nodes:
        node_host = "localhost"
        node_port = test_instance._nodes[node_index]._port_start

        api = LedgerApi(node_host, node_port)

        # Verify TXs - will block until they have executed
        for tx, identity, balance in tx_and_identity:

            # Check TX has executed, unless we expect it should already have been mined
            while True:
                status = api.tx.status(tx)

                if status == "Executed" or expect_mined:
                    break

                time.sleep(0.5)
                output("Waiting for TX to get executed. Found: {}".format(status))

            seen_balance = api.tokens.balance(identity)
            if balance != seen_balance:
                output(
                    "Balance mismatch found after sending to node. Found {} expected {}".format(
                        seen_balance, balance))
                test_instance._watchdog.trigger()

            output("Verified a wealth of {}".format(seen_balance))

        output("Verified balances for node: {}".format(node_index))

def restart_nodes(parameters, test_instance):

    nodes = parameters["nodes"]

    for node_index in nodes:
        test_instance.restart_node(node_index)

def add_node(parameters, test_instance):

    index = parameters["index"]
    node_connections = parameters["node_connections"]

    test_instance.append_node(index)
    test_instance.connect_nodes(node_connections)
    test_instance.start_node(index)


def run_steps(test_yaml, test_instance):
    output("Running steps: {}".format(test_yaml))

    for step in test_yaml:
        output("Running step: {}".format(step))

        command = ""
        parameters = ""

        if isinstance(step, dict):
            command = list(step.keys())[0]
            parameters = step[command]
        elif isinstance(step, str):
            command = step
        else:
            raise RuntimeError(
                "Failed to parse command from step: {}".format(step))

        if command == 'send_txs':
            send_txs(parameters, test_instance)
        elif command == 'verify_txs':
            verify_txs(parameters, test_instance)
        elif command == 'add_node':
            add_node(parameters, test_instance)
        elif command == 'sleep':
            time.sleep(parameters)
        elif command == 'print_time_elapsed':
            test_instance.print_time_elapsed()
        elif command == 'run_python_test':
            run_python_test(parameters, test_instance)
        elif command == 'restart_nodes':
            restart_nodes(parameters, test_instance)
        else:
            output(
                "Found unknown command when running steps: '{}'".format(
                    command))
            sys.exit(1)


def run_test(build_directory, yaml_file, constellation_exe):

    # Read YAML file
    with open(yaml_file, 'r') as stream:
        try:
            all_yaml = yaml.load_all(stream)

            # Parse yaml documents as tests (sequentially)
            for test in all_yaml:
                # Create a new test instance
                description = extract(test, 'test_description')
                output("\nTest: {}".format(description))

                if "DISABLED" in description:
                    output("Skipping disabled test")
                    continue

                # Create a test instance
                test_instance = TestInstance(
                    build_directory, constellation_exe, yaml_file)

                # Configure the test - this will start the nodes asynchronously
                setup_test(extract(test, 'setup_conditions'), test_instance)

                # Run the steps in the test
                run_steps(extract(test, 'steps'), test_instance)

                test_instance.stop()
        except Exception as e:
            print('Failed to parse yaml or to run test! Error: "{}"'.format(str(e)))
            traceback.print_exc()
            test_instance.stop()
            test_instance.dump_debug()
            sys.exit(1)

    output("\nAll end to end tests have passed :)")


def parse_commandline():
    parser = argparse.ArgumentParser(
        description='High level end to end tests reads a yaml file, and runs the tests within. Returns 1 if failed')

    # Required argument
    parser.add_argument(
        'build_directory', type=str,
        help='Location of the build directory relative to current path')
    parser.add_argument(
        'constellation_exe', type=str,
        help='Location of the constellation binary relative to current path')
    parser.add_argument('yaml_file', type=str,
                        help='Location of the yaml file dictating the tests')

    return parser.parse_args()


def main():
    args = parse_commandline()

    return run_test(args.build_directory, args.yaml_file,
                    args.constellation_exe)


if __name__ == '__main__':
    main()
