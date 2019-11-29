#!/usr/bin/env python3

#
# end to end tests of fetch-ledger
#
# This is achieved by using available fetch APIs to spin up a network locally
# and test it can handle certain conditions (such as single or multiple
# node failure)

import argparse
import codecs
import datetime
import glob
import importlib
import os
import pickle
import shutil
import subprocess
import sys
import threading
import time
import traceback
import json
import yaml
from threading import Event
from pathlib import Path
from threading import Event

from fetch.testing.testcase import ConstellationTestCase, DmlfEtchTestCase
from fetch.cluster.utils import output, verify_file, yaml_extract

from fetchai.ledger.api import LedgerApi
from fetchai.ledger.crypto import Entity

from .smart_contract_tests.synergetic_utils import SynergeticContractTestHelper


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


def create_test(setup_conditions, build_directory, node_exe, yaml_file):
    test_type = yaml_extract(
        setup_conditions, 'test_type', expected=False, expect_type=str)

    # default case
    if test_type is None:
        return ConstellationTestCase(build_directory, node_exe, yaml_file)

    if test_type == "Constellation":
        return ConstellationTestCase(build_directory, node_exe, yaml_file)
    elif test_type == "DmlfEtch":
        return DmlfEtchTestCase(build_directory, node_exe, yaml_file)
    else:
        raise ValueError("Unkown test type: {}".format(test_type))


def setup_test(test_yaml, test_instance):
    output("Setting up test: {}".format(test_yaml))

    test_name = yaml_extract(test_yaml, 'test_name',
                             expected=True, expect_type=str)
    number_of_nodes = yaml_extract(
        test_yaml, 'number_of_nodes', expected=True, expect_type=int)
    node_load_directory = yaml_extract(
        test_yaml, 'node_load_directory', expected=False, expect_type=dict)
    node_connections = yaml_extract(
        test_yaml, 'node_connections', expected=False, expect_type=list)
    mining_nodes = yaml_extract(test_yaml, 'mining_nodes',
                                expected=False, expect_type=list, default=[])
    max_test_time = yaml_extract(test_yaml, 'max_test_time',
                                 expected=False, expect_type=int, default=10)
    pos_mode = yaml_extract(test_yaml, 'pos_mode', expected=False,
                            expect_type=bool, default=False)
    num_lanes = yaml_extract(test_yaml, 'lanes', expected=False,
                             expect_type=int, default=1)

    test_instance._number_of_nodes = number_of_nodes
    test_instance._node_load_directory = node_load_directory
    test_instance._node_connections = node_connections
    test_instance._nodes_are_mining = mining_nodes
    test_instance._max_test_time = max_test_time
    test_instance._pos_mode = pos_mode
    test_instance._lanes = num_lanes

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

    sys.path.append(os.path.join(
        os.path.abspath(os.path.dirname(__file__)), "../"))
    test_script = importlib.import_module(
        parameters['script'], 'end_to_end_test')
    test_script.run({
        'host': host,
        'port': port
    })


def run_dmlf_etch_client(parameters, test_instance):
    indexes = parameters["nodes"]
    client_exe = parameters["exe"]
    etch_file = parameters["etch"]

    dmlf_etch_nodes = []
    for index in indexes:
        dmlf_etch_nodes.append(test_instance._nodes[index])

    build_directory = os.path.abspath(test_instance._build_directory)

    # get dmlf etch client executable
    client_exe = os.path.abspath(os.path.join(build_directory, client_exe))
    verify_file(client_exe)

    # get etch file
    etch_file = os.path.abspath(os.path.join(build_directory, etch_file))
    verify_file(etch_file)

    client_output_dir = os.path.abspath(
        os.path.join(test_instance._workspace, "steps/"))
    os.makedirs(client_output_dir, exist_ok=True)

    # generate config file
    config_path = os.path.join(client_output_dir, "e2e_config_client.json")
    nodes = [
        {
            "uri": node.uri,
            "pub": node.public_key
        }
        for node in dmlf_etch_nodes
    ]

    key = Entity()
    config = {
        "client": {
            "key": key.private_key
        },
        "nodes": nodes
    }

    with open(config_path, 'w') as f:
        json.dump(config, f)

    # generate client command
    cmd = [client_exe, config_path, etch_file]

    # run client
    logfile_path = os.path.join(client_output_dir, "dmlf_etch_client.log")
    logfile = open(logfile_path, 'w')
    subprocess.check_call(cmd, cwd=build_directory,
                          stdout=logfile, stderr=subprocess.STDOUT)


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
            error_message = ""

            # Check TX has executed, unless we expect it should already have been mined
            while True:
                status = api.tx.status(tx).status

                if status == "Executed" or expect_mined:
                    output("found executed TX")
                    error_message = ""
                    break

                tx_b64 = codecs.encode(codecs.decode(
                    tx, 'hex'), 'base64').decode()

                next_error_message = "Waiting for TX to get executed (node {}). Found: {} Tx: {}".format(
                    node_index, status, tx_b64)

                time.sleep(0.5)

                if next_error_message != error_message:
                    output(next_error_message)
                    error_message = next_error_message

            failed_to_find = 0

            while True:
                seen_balance = api.tokens.balance(identity)

                # There is an unavoidable race that can cause you to see a balance of 0
                # since the TX can be lost even after supposedly being executed.
                if seen_balance == 0 and balance is not 0:
                    output(
                        f"Note: found a balance of 0 when expecting {balance}. Retrying.")

                    time.sleep(1)

                    failed_to_find = failed_to_find + 1

                    if failed_to_find > 5:
                        # Forces the resubmission of wealth TX to the chain (TX most likely was lost)
                        api.tokens.wealth(identity, balance)
                        failed_to_find = 0
                else:
                    # Non-zero balance at this point. Stop waiting.
                    if balance != seen_balance:
                        output(
                            "Balance mismatch found after sending to node. Found {} expected {}".format(
                                seen_balance, balance))
                        test_instance._watchdog.trigger()
                    break

            output("Verified a wealth of {}".format(seen_balance))

        output("Verified balances for node: {}".format(node_index))


def get_nodes_private_key(test_instance, index):
    # Path to config files (should already be generated)
    expected_ouptut_dir = os.path.abspath(
        os.path.dirname(test_instance._yaml_file) + "/input_files")

    key_path = expected_ouptut_dir + "/{}.key".format(index)
    verify_file(key_path)

    private_key = open(key_path, "rb").read(32)

    return private_key


def destake(parameters, test_instance):
    nodes = parameters["nodes"]

    for node_index in nodes:
        node_host = "localhost"
        node_port = test_instance._nodes[node_index]._port_start

        # create the API objects we use to interface with the nodes
        api = LedgerApi(node_host, node_port)

        # create the entity from the node's private key
        entity = Entity(get_nodes_private_key(test_instance, node_index))

        current_stake = api.tokens.stake(entity)

        output(f'Destaking node {node_index}. Current stake: ', current_stake)
        output(
            f'Destaking node {node_index}. Current balance: ', api.tokens.balance(entity))

        api.sync(api.tokens.add_stake(entity, 1, 500))
        api.sync(api.tokens.de_stake(entity, current_stake, 500))
        api.sync(api.tokens.collect_stake(entity, 500))

        output(f'Destaked node {node_index}. Current stake: ', current_stake)
        output(
            f'Destaked node {node_index}. Current balance: ', api.tokens.balance(entity))
        output(f'Destaked node {node_index}. Current cooldown stake: ',
               api.tokens.stake_cooldown(entity))


def restart_nodes(parameters, test_instance):
    nodes = parameters["nodes"]
    remove_dbs = parameters.get("remove_dbs", False)

    for node_index in nodes:
        test_instance.restart_node(node_index, remove_dbs)

    time.sleep(5)


def stop_nodes(parameters, test_instance):
    nodes = parameters["nodes"]
    remove_dbs = parameters.get("remove_dbs", False)

    for node_index in nodes:
        test_instance.stop_node(node_index, remove_dbs)

    time.sleep(5)


def start_nodes(parameters, test_instance):
    nodes = parameters["nodes"]

    for node_index in nodes:
        test_instance.start_node(node_index)

    time.sleep(5)


def add_node(parameters, test_instance):
    index = parameters["index"]
    node_connections = parameters["node_connections"]

    test_instance.append_node(index)
    test_instance.connect_nodes(node_connections)
    test_instance.start_node(index)


def create_wealth(parameters, test_instance):
    nodes = parameters["nodes"]
    amount = parameters["amount"]

    for node_index in nodes:
        node_host = "localhost"
        node_port = test_instance._nodes[node_index]._port_start

        api = LedgerApi(node_host, node_port)

        # create the entity from the node's private key
        entity = Entity(get_nodes_private_key(test_instance, node_index))
        tx = api.tokens.wealth(entity, amount)
        for i in range(10):
            output('Create balance of: ', amount)
            api.sync(tx, timeout=120, hold_state_sec=20)
            for j in range(5):
                b = api.tokens.balance(entity)
                output('Current balance: ', b)
                if b >= amount:
                    return
                time.sleep(5)
            time.sleep(5)
        raise Exception("Failed to create wealth")


def create_synergetic_contract(parameters, test_instance):
    nodes = parameters["nodes"]
    name = parameters["name"]
    fee_limit = parameters["fee_limit"]
    for node_index in nodes:
        node_host = "localhost"
        node_port = test_instance._nodes[node_index]._port_start

        api = LedgerApi(node_host, node_port)

        # create the entity from the node's private key
        entity = Entity(get_nodes_private_key(test_instance, node_index))
        output('Create contract, available balance: ',
               api.tokens.balance(entity))
        helper = SynergeticContractTestHelper(
            name, api, entity, test_instance._workspace)
        helper.create_new(fee_limit)
        test_instance._nodes[node_index]._contract = helper


def run_contract(parameters, test_instance):
    nodes = parameters["nodes"]
    contract_name = parameters["contract_name"]
    wait_for_blocks_num = parameters["wait_for_blocks"]
    for node_index in nodes:
        node_host = "localhost"
        node_port = test_instance._nodes[node_index]._port_start

        api = LedgerApi(node_host, node_port)

        # create the entity from the node's private key
        entity = Entity(get_nodes_private_key(test_instance, node_index))

        try:
            contract_helper = test_instance._nodes[node_index]._contract
        except AttributeError:
            output(
                f"No contract stored in test_instance (node_index={node_index})! Loading from file...")
            contract_helper = SynergeticContractTestHelper(
                contract_name, api, entity, test_instance._workspace)
            contract_helper.load()
        output('Submit data, available balance: ', api.tokens.balance(entity))
        contract_helper.submit_random_data(10, (0, 200))
        api.wait_for_blocks(wait_for_blocks_num)
        valid = contract_helper.validate_execution()
        if not valid:
            output(
                f"Synergetic contract ({contract_name}) execution failed on node {node_index}!")
            raise Exception(
                f"Synergetic contract ({contract_name}) execution failed on node {node_index}!")
        else:
            output(
                f"Synergetic contract ({contract_name}) executed on node {node_index} ")


def wait_for_blocks(parameters, test_instance):
    nodes = parameters["nodes"]
    wait_for_blocks_num = parameters["num"]
    for node_index in nodes:
        test_instance.wait_for_blocks(node_index, wait_for_blocks_num)


def verify_chain_sync(parameters, test_instance):
    max_trials = parameters.get("max_trials", 20)
    node_idx = parameters["node"]
    output(f"verify_chain_sync: node={node_idx}")

    if not test_instance.verify_chain_sync(node_idx, max_trials):
        raise RuntimeError(
            f"Node {node_idx} chain not synced with the network!")


def wait_network_ready(parameters, test_instance):
    sleep_time = parameters.get("sleep", 2)
    max_trials = parameters.get("max_trials", 20)
    for i in range(max_trials):
        try:
            if test_instance.network_ready():
                return
        except Exception as e:
            print("Exception: ", e)
        time.sleep(sleep_time)
    raise RuntimeError(
        f"Network readiness check failed, because reached max trials ({max_trials})")


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
        elif command == 'stop_nodes':
            stop_nodes(parameters, test_instance)
        elif command == 'start_nodes':
            start_nodes(parameters, test_instance)
        elif command == 'destake':
            destake(parameters, test_instance)
        elif command == 'run_dmlf_etch_client':
            run_dmlf_etch_client(parameters, test_instance)
        elif command == "create_wealth":
            create_wealth(parameters, test_instance)
        elif command == "create_synergetic_contract":
            create_synergetic_contract(parameters, test_instance)
        elif command == "run_contract":
            run_contract(parameters, test_instance)
        elif command == "wait_for_blocks":
            wait_for_blocks(parameters, test_instance)
        elif command == "verify_chain_sync":
            verify_chain_sync(parameters, test_instance)
        elif command == "wait_network_ready":
            wait_network_ready(parameters, test_instance)
        else:
            output(
                "Found unknown command when running steps: '{}'".format(
                    command))
            sys.exit(1)


def run_test(build_directory, yaml_file, node_exe, name_filter=None):

    # Read YAML file
    with open(yaml_file, 'r') as stream:
        try:
            all_yaml = yaml.safe_load_all(stream)

            # Parse yaml documents as tests (sequentially)
            for test in all_yaml:
                # Get test setup conditions
                setup_conditions = yaml_extract(test, 'setup_conditions')

                # Check if name is not filtered out
                if name_filter is not None:
                    name = yaml_extract(setup_conditions, 'test_name')
                    if name not in name_filter:
                        continue

                # Create a new test instance
                description = yaml_extract(test, 'test_description')
                output("\n=================================================")
                output("Test: {}".format(description))
                output("=================================================\n")

                if "DISABLED" in description:
                    output("Skipping disabled test")
                    continue

                # Create a test instance
                test_instance = create_test(
                    setup_conditions, build_directory, node_exe, yaml_file)

                # Configure the test - this will start the nodes asynchronously
                setup_test(setup_conditions, test_instance)

                # Run the steps in the test
                run_steps(yaml_extract(test, 'steps'), test_instance)

                test_instance.stop()
        except Exception as e:
            print('Failed to parse yaml or to run test! Error: "{}"'.format(e))
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
        'node_exe', type=str,
        help='Location of the application binary to deploy relative to current path (should be compatible with the test_type in the yaml_file)')
    parser.add_argument('yaml_file', type=str,
                        help='Location of the yaml file dictating the tests')

    return parser.parse_args()


def main():
    args = parse_commandline()

    return run_test(args.build_directory, args.yaml_file,
                    args.node_exe)


if __name__ == '__main__':
    main()
