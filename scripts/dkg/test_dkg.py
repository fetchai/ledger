#!/usr/bin/env python3

#
# Test the DKG as a standalone app.
#
# This file will run the DKG app(s) passing in the network details and public keys of their peers


import sys
import os
import argparse
import fnmatch
import time
import pprint
import threading

# The fetch library for managing multiple executables at once
from fetch.cluster.instance import Instance
from fetch.cluster.monitor import MonitoredLogFile


def print_json(arg):
    pp = pprint.PrettyPrinter(indent=4)
    pp.pprint(arg)

# Custom class for this example


class DKGInstance(Instance):
    def __init__(self, app_path, port_start, root, clear_path=True):
        self._app_path = str(app_path)
        self._port_start = int(port_start)

        # Ensure root dir exists
        if not os.path.exists(root):
            os.makedirs(root)

        # fake construct the instances
        super().__init__([], root)

        # Run the program with no arguments to get the public key
        self._cmd = [self._app_path, ]

        # if requested clear the current directory
        if clear_path:
            for ext in ('*.db', '*.log'):
                for item in fnmatch.filter(os.listdir(root), ext):
                    os.remove(os.path.join(root, item))

        self.start()
        time.sleep(1)
        self.stop()

        # First line of DKG exe MUST be only the key
        with open(self._log_path) as f:
            self._pub_key_b64 = f.readline().strip('\n')

        print("Found pub key for node: ", self._pub_key_b64)

    def start(self):
        super().start()

        print('Started DKG instance {} on pid {}'.format(
            self._port_start, self._process.pid))
        print_json(self._cmd)
        print("")

    def configure_cmd_for_demo(self, node_list, threshold):
        # args shall be : ./exe beacon_address port threshold peer1 priv_key_b64 peer2 priv_key_b64...
        self._cmd = [self._app_path, ]
        self._cmd.append(node_list[0]._pub_key_b64)     # beacon address
        self._cmd.append(str(self._port_start))  # this port
        self._cmd.append(str(threshold))                # threshold

        for node in node_list:
            if node is self:
                print("found self")
            else:
                # other peer IP
                self._cmd.append("tcp://127.0.0.1:"+str(node._port_start))
                # other peer address
                self._cmd.append(node._pub_key_b64)


class LogFileFollower(object):
    def __init__(self):
        self._running = False
        self._thread = None

    def start(self, log_file):

        # kick off the thread
        self._log_file = log_file
        self._running = True
        self._thread = threading.Thread(target=self._background_monitor)
        self._thread.daemon = True
        self._thread.start()

    def stop(self):
        self._running = False
        self._thread.join()

    def _background_monitor(self):

        # create the log file watchers
        monitors = [MonitoredLogFile(self._log_file, ""), ]

        while self._running:
            for monitor in monitors:
                monitor.update()

            time.sleep(0.35)


def run_test(dkg_exe, output_directory, number_nodes, threshold):

    # Ensure output dir exists
    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

    dkg_exe = os.path.abspath(dkg_exe)

    start_port = 8000
    DKG_nodes = []

    # Create DKG nodes
    for index in range(number_nodes):
        DKG_nodes.append(DKGInstance(dkg_exe, start_port,
                                     output_directory+"/"+str(start_port)))
        start_port = start_port + 1

    # Start DKG nodes in reverse
    for i in range(len(DKG_nodes)-1, -1, -1):
        DKG_nodes[i].configure_cmd_for_demo(DKG_nodes, threshold)
        DKG_nodes[i].start()

    follow = LogFileFollower()
    follow.start(DKG_nodes[0]._log_path)

    while DKG_nodes[0]._process.poll() == None:
        time.sleep(2)

    follow.stop()
    print("Finished. Actually quitting")
    sys.exit(0)


def parse_commandline():

    parser = argparse.ArgumentParser(
        description='High level DKG test')

    # Required arguments
    parser.add_argument(
        'dkg_exe', type=str,
        help='Location of the dkg binary')

    parser.add_argument(
        'output_directory', type=str,
        help='directory to put output log files in')

    parser.add_argument('number_nodes', type=int,
                        help='Number of nodes to create')

    parser.add_argument('threshold', type=int,
                        help='Threshold for number of nodes to be honest')

    return parser.parse_args()


def main():
    args = parse_commandline()

    return run_test(args.dkg_exe, args.output_directory, args.number_nodes, args.threshold)


if __name__ == '__main__':
    main()
