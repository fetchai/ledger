#!/usr/bin/env python3

# pip3 install fetchai-ledger-api

import sys
import json
import subprocess

from fetchai.ledger.crypto import Entity, Address


def main():
    application_binary = "./cmake-build-release/libs/ml/examples/example-ml-mnist-multiprocess-distributed-learning"
    data1 = "/home/jiri/workspace/mnist/t10k-images.idx3-ubyte"
    data2 = "/home/jiri/workspace/mnist/t10k-labels.idx1-ubyte"

    count = 4

    idents = [
        (x, Entity())
        for x
        in range(0, int(count))
    ]
    peers = [
        {
            "uri": "tcp://127.0.0.1:{}".format(8000 + instance_number),
            "key": ident.private_key,
            "pub": ident.public_key,
        }
        for instance_number, ident
        in idents
    ]
    config = {
        "peers": peers
    }

    config_str = json.dumps(config)

    process_lines = [
        [
            application_binary,
            data1,
            data2,
            config_str,
            str(instance_number),
        ]
        for instance_number
        in range(0, len(peers))
    ]

    processes = [
        subprocess.Popen(pl)
        for pl
        in process_lines
    ]

    results = [
        x.communicate()
        for x
        in processes
    ]

    if all([process.returncode == 0 for process in processes]):
        print("ok")
    else:
        print("failed")
        exit(1)


if __name__ == "__main__":
    main()
