import os
import json
import time
import datetime
import glob
import shutil
import subprocess
import sys
from pathlib import Path

from fetch.cluster.instance import ConstellationInstance, DmlfEtchInstance
from fetch.cluster.utils import output, verify_file
from fetch.cluster.chain import ChainSyncTesting

from fetchai.ledger.api.common import ApiEndpoint
from fetchai.ledger.api import LedgerApi
from fetchai.ledger.crypto import Entity
from fetchai.ledger.genesis import *
from fetch.cluster.chain import ChainSyncTesting


class TestCase(object):
    """
    """

    def append_node(self, index, load_directory=None):
        pass

    def connect_nodes(self, node_connections):
        pass

    def start_node(self, index):
        pass

    def stop_node(self, index, remove_db=False):
        pass

    def restart_node(self, index, remove_db=False):
        pass

    def setup_input_files(self):
        pass

    def run(self):
        pass

    def stop(self):
        pass

    def print_time_elapsed(self):
        pass

    def node_ready(self, index):
        pass

    def network_ready(self):
        pass

    def wait_for_blocks(self, node_index, number_of_blocks):
        pass

    def verify_chain_sync(self, node_index, max_trials, sleep_time):
        pass


class ConstellationTestCase(TestCase):
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
        self._block_interval = 1000

        # Variables related to temporary pos mode
        self._pos_mode = False
        self._nodes_pubkeys = []
        self._nodes_keys = []

        # Default to removing old tests
        for f in glob.glob(build_directory + "/end_to_end_test_*"):
            shutil.rmtree(f)

        # To avoid possible collisions, prepend output files with the date
        self._random_identifier = '{0:%Y_%m_%d_%H_%M_%S}'.format(
            datetime.datetime.now())

        self._random_identifier = "default"

        self._workspace = os.path.join(
            build_directory, 'end_to_end_test_{}'.format(
                self._random_identifier))
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

        # Possibly soon to be deprecated functionality - set the block interval
        instance.block_interval = self._block_interval
        instance.feature_flags = ['synergetic']

        # configure the lanes and slices
        instance.lanes = self._lanes
        instance.slices = self._slices

        assert len(self._nodes) == index, \
            "Attempt to add node with an index mismatch. Current len: {}, index: {}".format(
            len(self._nodes), index)

        self._nodes.append(instance)

    def connect_nodes(self, node_connections):
        for connect_from, connect_to in node_connections:
            self._nodes[connect_from].add_peer(self._nodes[connect_to])
            output("Connect node {} to {}".format(connect_from, connect_to))

    def start_node(self, index):
        if self._nodes[index].started:
            print('Node {} already started!'.format(index))
            return
        print('Starting Node {}...'.format(index))

        self._nodes[index].start()
        print('Starting Node {}...complete'.format(index))

        time.sleep(1)

    def setup_pos_for_nodes(self):

        nodes_identities = []

        # First give each node that is mining a unique identity
        for index in range(self._number_of_nodes):

            # max 200 mining nodes due to consensus requirements
            assert index <= 200

            node = self._nodes[index]

            if node.mining:
                print('Setting up POS for node {}...'.format(index))
                print('Giving node the identity: {}'.format(
                    node._entity.public_key))

            nodes_identities.append(
                (node._entity, 100, 10000 if node.mining else 0))

        genesis_file = GenesisFile(
            nodes_identities, 20, 5, self._block_interval)
        genesis_file_location = os.path.abspath(
            os.path.join(self._workspace, "genesis_file.json"))
        genesis_file.dump_to_file(genesis_file_location)

        # Give all nodes this stake file, plus append POS flag for when node starts
        for index in range(self._number_of_nodes):
            shutil.copy(genesis_file_location, self._nodes[index].root)
            self._nodes[index].append_to_cmd(["-pos", "-private-network", ])

    def restart_node(self, index, remove_db=False):
        print('Restarting Node {}...'.format(index))

        self._nodes[index].stop()

        # Optically remove db files when testing recovering from a genesis file
        if remove_db:
            # self.dump_debug(index)

            pattern = ["*.db"]
            for p in pattern:
                [os.remove(x) for x in glob.iglob('./**/' + p, recursive=True)]

        self.start_node(index)
        time.sleep(3)

    def remove_db_files(self, index):
        root = os.path.abspath(os.path.join(
            self._workspace, 'node{}'.format(index)))
        pattern = ["*.db"]
        for p in pattern:
            [os.remove(x)
             for x in glob.iglob(f"{root}/**/{p}", recursive=True)]
        for f in os.listdir(root):
            if f.find(".db") != -1:
                raise RuntimeError(f"Db files not removed for node {index}")

    def stop_node(self, index, remove_db=False):
        self._nodes[index].stop()
        if remove_db:
            self.remove_db_files(index)

    def print_time_elapsed(self):
        output("Elapsed time: {}".format(
            time.perf_counter() - self._creation_time))

    def run(self):

        # build up all the node instances
        for index in range(self._number_of_nodes):
            self.append_node(index, self._node_load_directory)

        # Now connect the nodes as specified
        if self._node_connections:
            self.connect_nodes(self._node_connections)

        # Enable mining node(s)
        for miner_index in self._nodes_are_mining:
            self._nodes[miner_index].mining = True

        # In the case only one miner node, it runs in standalone mode
        if len(self._nodes) == 1 and len(self._nodes_are_mining) > 0:
            self._nodes[0].standalone = True
        else:
            for node in self._nodes:
                node.private_network = True

        # Temporary special case for POS mode
        if self._pos_mode:
            self.setup_pos_for_nodes()

        # start all the nodes
        for index in range(self._number_of_nodes):
            if self._number_of_nodes > 1 and not self._pos_mode:
                self._nodes[index].append_to_cmd(["-private-network", ])
            self.start_node(index)

        sleep_time = 5 + (3 * self._lanes)

        if self._pos_mode:
            sleep_time *= 2
            output("POS mode. sleep extra time.")

        time.sleep(sleep_time)

    def stop(self):
        if self._nodes:
            for n, node in enumerate(self._nodes):
                print('Stopping Node {}...'.format(n))
                if node:
                    node.stop()
                print('Stopping Node {}...complete'.format(n))

        if self._watchdog:
            self._watchdog.stop()

    # If something goes wrong, print out debug state (mainly node log files)
    def dump_debug(self, only_node=None):
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

    def node_ready(self, index):
        try:
            port = self._nodes[index]._port_start
            api = ApiEndpoint("localhost", port)
            status, response = api._get_json("health/ready")
            if status:
                for key, value in response.items():
                    if not value:
                        output(
                            f"Node {index} not ready, because {key} is False!")
                        return False
                return True
        except Exception as e:
            output(f"Failed to call node {index}: {str(e)}")
        return False

    def network_ready(self):
        for i in range(len(self._nodes)):
            if not self.node_ready(i):
                return False
        return True

    def wait_for_blocks(self, node_index, number_of_blocks):
        """
        Wait for a specific number of blocks in the selected node
        :param node_index: which node we are interested in
        :param number_of_blocks: for how many new block to wait
        :return:
        """
        port = self._nodes[node_index]._port_start
        output(
            f"Waiting for {number_of_blocks} blocks on node {port}")
        api = LedgerApi("localhost", port)
        api.wait_for_blocks(number_of_blocks)

    def verify_chain_sync(self, node_index, max_trials=20):
        """
        Verify if a node has synced it's chain with the rest of the network
        :param node_index: which node we want to verify
        :param max_trials: maximum of how many times we try the sync test
        :return:
        """
        config = []
        for node in self._nodes:
            node_host = "localhost"
            node_port = node._port_start
            config.append({
                "host": node_host,
                "port": node_port
            })
        sync_test = ChainSyncTesting(config)
        target_host = "localhost"
        target_port = self._nodes[node_index]._port_start
        sleep_time = self._nodes[node_index].block_interval*1.2/1000.
        for i in range(max_trials):
            try:
                if sync_test.node_synced(target_host, target_port):
                    output(f"Node {node_index} chain synced with the network!")
                    return True
            except Exception as e:
                output(f"verify_chain_sync exception: {e}")
            time.sleep(sleep_time)
        return False


class DmlfEtchTestCase(TestCase):
    """
    Sets up an instance of a test, containing references to started nodes and other relevant data
    """

    def __init__(self, build_directory, node_exe, yaml_file):

        self._number_of_nodes = 0
        self._node_connections = None
        self._port_start_range = 8000
        self._port_range = 20
        self._workspace = ""
        self._max_test_time = 1000
        self._nodes = []
        self._watchdog = None
        self._creation_time = time.perf_counter()
        self._nodes_pubkeys = []
        self._nodes_keys = []

        # Default to removing old tests
        for f in glob.glob(build_directory + "/end_to_end_test_*"):
            shutil.rmtree(f)

        # To avoid possible collisions, prepend output files with the date
        self._random_identifer = '{0:%Y_%m_%d_%H_%M_%S}'.format(
            datetime.datetime.now())

        self._random_identifer = "default"

        self._workspace = os.path.join(
            build_directory, 'end_to_end_test_{}'.format(
                self._random_identifer))
        self._build_directory = build_directory
        self._node_exe = os.path.abspath(node_exe)
        self._yaml_file = os.path.abspath(yaml_file)
        self._test_files_dir = os.path.dirname(self._yaml_file)

        verify_file(node_exe)
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
        key = self._nodes_keys[index]
        pub = self._nodes_pubkeys[index]

        # Create an instance of the constellation - note we don't clear path since
        # it should be clear unless load_directory is used
        instance = DmlfEtchInstance(
            self._node_exe,
            port,
            key,
            pub,
            root,
            clear_path=False
        )

        assert len(self._nodes) == index, "Attempt to add node with an index mismatch. Current len: {}, index: {}".format(
            len(self._nodes), index)

        self._nodes.append(instance)

    def setup_nodes_keys(self):

        # Give each node a unique identity
        for index in range(self._number_of_nodes):
            entity = Entity()
            pub64 = entity.public_key
            key64 = entity.private_key

            print('Setting up key for node {}...'.format(index))
            print('Giving node the identity: {}'.format(pub64))
            self._nodes_pubkeys.append(pub64)

            print('Giving node key: {}'.format(key64))
            self._nodes_keys.append(key64)

    def connect_nodes(self, node_connections):
        for connect_from, connect_to in node_connections:
            self._nodes[connect_from].add_peer(self._nodes[connect_to])
            output("Connect node {} to {}".format(connect_from, connect_to))

    def start_node(self, index):
        print('Starting Node {}...'.format(index))

        self._nodes[index].start()
        print('Starting Node {}...complete'.format(index))

        time.sleep(1)

    def restart_node(self, index, remove_db=False):
        print('Restarting Node {}...'.format(index))

        self._nodes[index].stop()

        # Optically remove db files when testing recovering from a genesis file
        if remove_db:
            self.dump_debug(index)

            pattern = ["*.db"]
            for p in pattern:
                [os.remove(x) for x in glob.iglob('./**/' + p, recursive=True)]

        self.start_node(index)
        time.sleep(3)

    def stop_node(self, index, remove_db=False):
        output("Not implemented")

    def print_time_elapsed(self):
        output("Elapsed time: {}".format(
            time.perf_counter() - self._creation_time))

    def run(self):

        # setup node keys
        self.setup_nodes_keys()

        # build up all the node instances
        for index in range(self._number_of_nodes):
            self.append_node(index, self._node_load_directory)

        # Now connect the nodes as specified
        if self._node_connections:
            self.connect_nodes(self._node_connections)

        # start all the nodes
        for index in range(self._number_of_nodes):
            self.start_node(index)

        time.sleep(5)  # TODO(HUT): blocking http call to node for ready state

    def stop(self):
        if self._nodes:
            for n, node in enumerate(self._nodes):
                print('Stopping Node {}...'.format(n))
                if(node):
                    node.stop()
                print('Stopping Node {}...complete'.format(n))

        if self._watchdog:
            self._watchdog.stop()

    # If something goes wrong, print out debug state (mainly node log files)
    def dump_debug(self, only_node=None):
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

    def node_ready(self, index):
        return True

    def network_ready(self):
        return True

    def wait_for_blocks(self, node_index, number_of_blocks):
        pass

    def verify_chain_sync(self, node_index, max_trials, sleep_time):
        pass
