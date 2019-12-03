import argparse
import os
import random
import sys
import time
from fetch.cluster.instance import ConstellationInstance
from fetch.cluster.monitor import ConstellationMonitor

HEADER_TEMPLATE = """F E ╱    {name}
  T C    Copyright 2018 (c) Fetch AI Ltd.
    H
"""


def interconnect(text):
    text = text.lower()
    valid_interconnects = ('chain', 'chaos')
    if text not in valid_interconnects:
        raise RuntimeError(
            'Invalid interconnection. Choose on of: ' + ','.join(valid_interconnects))
    return text


def app_path(path):
    abs_path = os.path.abspath(path)
    if not os.path.isfile(abs_path):
        raise RuntimeError(
            'Unable to locate requested application: {}'.format(abs_path))
    return abs_path


def parse_commandline():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()

    parser_run = subparsers.add_parser('run', help='Run a swarm locally')
    parser_run.add_argument('application', type=app_path,
                            metavar='APP', help='The path to the application')
    parser_run.add_argument('workspace', metavar='PATH',
                            help='The path to the workspace to run nodes in')
    parser_run.add_argument('network_size', type=int,
                            metavar='N', help='The number of network nodes')
    parser_run.add_argument('-l', '--lanes', type=int,
                            help='The number of lanes to be used')
    parser_run.add_argument('-s', '--slices', type=int,
                            help='The number of slcies to be used')
    parser_run.add_argument('--start-port', type=int,
                            default=8000, help='The port to start assigning from')
    parser_run.add_argument('--port-range', type=int, default=20,
                            help='The number of ports assigned to each node')
    parser_run.add_argument('-I', '--interconnect', default='chain', type=interconnect,
                            help='The strategy for connecting the peers together')
    parser_run.add_argument('--chain-length', type=int, default=2,
                            help='The number of connections in the chain to make')
    parser_run.add_argument('-A', '--chaos-size', type=int, default=2,
                            help='The desired number of connections for each node')
    parser_run.add_argument(
        '-f', '--follow', action='store_true', help='Display the status of the nodes')
    parser_run.add_argument('--block-interval', type=int,
                            default=5000, help='The block interval for the mining node')
    parser_run.set_defaults(handler=run)

    return parser, parser.parse_args()


def run(args):
    # extract common parameters
    port_start_range = args.start_port
    port_range = args.port_range

    # adjust the port range if needed
    if args.lanes:
        estimated_port_range = 10 + args.lanes
        port_range = max(port_range, estimated_port_range)

    monitor = None
    nodes = []

    # build up all the instances
    for n in range(args.network_size):
        root = os.path.abspath(os.path.join(
            args.workspace, 'node{}'.format(n + 1)))
        port = port_start_range + (port_range * n)

        # ensure the workspace folder exits
        os.makedirs(root, exist_ok=True)

        instance = ConstellationInstance(
            args.application,
            port,
            root
        )

        # configure the lanes and slices if needed
        if args.lanes:
            instance.lanes = args.lanes
        if args.slices:
            instance.slices = args.slices

        nodes.append(instance)

    if args.interconnect == 'chain':

        # inter connect the nodes (single chain)
        for n in range(len(nodes)):
            for m in range(args.chain_length):
                o = n - (m + 1)
                if o >= 0:
                    nodes[n].add_peer(nodes[o])

    elif args.interconnect == 'chaos':
        all_nodes = set(list(range(len(nodes))))

        connections_pairs = set()

        for n in all_nodes:
            possible_nodes = list()
            for m in all_nodes - {n}:
                if (n, m) in connections_pairs or (m, n) in connections_pairs:
                    continue
                possible_nodes.append(m)

            if len(possible_nodes) == 0:
                raise RuntimeError(
                    'Chaos size too large to support this network, please choose a lower number')
            elif len(possible_nodes) > args.chaos_size:
                random.shuffle(possible_nodes)
                possible_nodes = possible_nodes[:args.chaos_size]

            for m in possible_nodes:
                connections_pairs.add((n, m))

        # build a debug map and print it
        connection_map = {}
        for n, m in connections_pairs:
            connections = connection_map.get(n, set())
            connections.add(m)
            connection_map[n] = connections
        for n, ms in connection_map.items():
            print('- {} -> {}'.format(n, ','.join(map(str, ms))))

        # make the connections
        for n, m in connections_pairs:
            nodes[n].add_peer(nodes[m])

    else:
        assert False

    # ensure the first node is always mining
    nodes[0].block_interval = args.block_interval

    # It needs to know if it is the only node in the network
    if len(nodes) == 1:
        nodes[0].standalone = True

    # add nodes to monitor list
    if args.follow:
        monitor = ConstellationMonitor(nodes)

    # start all the nodes
    for n, node in enumerate(nodes):
        print('Starting Node {}...'.format(n))
        node.start()
        print('Starting Node {}...complete'.format(n))

        # allow graceful startup of the network
        if args.interconnect == 'chain':
            time.sleep(1.0)

    # wait for the first one to fail
    try:

        # allow the monitor to start
        if monitor is not None:
            monitor.start()

        monitoring = True
        while monitoring:

            # evaluate all the nodes
            for node in nodes:
                exit_code = node.poll()
                if exit_code is not None:
                    monitoring = False
                    break

            # wait for a period
            time.sleep(0.3)

    except KeyboardInterrupt:
        pass  # normal fail case

    # stop all the nodes
    for n, node in enumerate(nodes):
        print('Stopping Node {}...'.format(n))
        node.stop()
        print('Stopping Node {}...complete'.format(n))


def main():
    print(HEADER_TEMPLATE.format(name='Swarm'))

    # parse the command line
    parser, args = parse_commandline()

    # ensure that a correct command has been set
    if hasattr(args, 'handler'):
        exit_code = args.handler(args)
        if isinstance(exit_code, int):
            sys.exit(exit_code)
    else:
        parser.print_usage()
        sys.exit(1)
