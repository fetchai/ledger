#!/usr/bin/env python3
import os
import fnmatch
import argparse
import subprocess
import time


TEMP_FILES = ('*.db',)
PORT_RANGE = 50
CHAIN_LEN = 1


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('app_path', help='The path to the application to run')
    parser.add_argument('num_nodes', metavar='N', type=int, help='The number of nodes to launch')
    parser.add_argument('-w', '--working-folder', default='nodes', help='Path to nodes root')
    parser.add_argument('--port-start', default=10000, type=int, help='The starting port offset')
    return parser.parse_args()


def main():
    args = parse_commandline()

    ### Step 1. Setup

    # create all the nodes
    node_config = []
    for n in range(args.num_nodes):

        # define and create the workspace root
        node_workspace = os.path.join(args.working_folder, 'node{}'.format(n + 1))
        os.makedirs(node_workspace, exist_ok=True)

        # clear any of the database files from the directory
        contents = os.listdir(node_workspace)
        for pattern in TEMP_FILES:
            for path in fnmatch.filter(contents, pattern):
                os.remove(os.path.join(node_workspace, path))

        # define the port config
        node_port_offset = n * PORT_RANGE + args.port_start
        log_path = os.path.join(node_workspace, 'node.log')

        peers = list(
            map(
                lambda x: '127.0.0.1:{}'.format((x * PORT_RANGE) + args.port_start + 1),
                filter(lambda x: x >= 0, [n - m for m in range(1, CHAIN_LEN + 1)])
            )
        )

        cmd = [
            os.path.abspath(args.app_path),
            '-port', str(node_port_offset),
            '-interface', '127.0.0.1',
        ]

        if len(peers) > 0:
            cmd += ['-peers', ','.join(peers)]

        node_config.append((n, node_port_offset, node_workspace, log_path, peers, cmd))

    ### Step 2. Launch
    log_files = []
    nodes = []
    for n, _, workspace, log_path, _, cmd in node_config:

        # create the log
        log = open(log_path, 'w')
        log_files.append(log)

        if n == 0:
            nodes.append(None)
            continue

        # create the process
        proc = subprocess.Popen(cmd, cwd=workspace, stdout=log, stderr=subprocess.STDOUT)
        nodes.append(proc)

    ### Step 3. Monitor
    exit_codes = [None] * len(nodes)
    try:
        while True:

            num_active = sum(map(lambda x: 1 if x is None else 0, exit_codes))
            num_failed = len(exit_codes) - num_active

            if num_failed > 0:
                print('1 or node nodes failed')
                break

            print('Nodes active: {}'.format(num_active))

            for n, node in enumerate(nodes):
                if node is None:
                    continue

                # update the exit codes
                if exit_codes[n] is None:
                    exit_codes[n] = node.poll()

                    # detect exit code change
                    if exit_codes[n] is not None:
                        print('Node {} exited with code {}'.format(n, exit_codes[n]))

            time.sleep(1)
    except:
        pass

    ### Step 4. Tear Down
    for node, exit_code, log in zip(nodes, exit_codes, log_files):
        if exit_code is None and node is not None:
            node.kill()

        log.write('\n\nShutdown\n\nExit code was: {}\n\n'.format(exit_code))
        log.flush()
        log.close()




if __name__ == '__main__':
    main()
