#!/usr/bin/env python3

import subprocess


def main():
    exename = "./cmake-build-release/libs/ml/examples/example-ml-mnist-multiprocess-distributed-learning"
    data1 = "/home/jiri/workspace/mnist/t10k-images.idx3-ubyte"
    data2 = "/home/jiri/workspace/mnist/t10k-labels.idx1-ubyte"

    names = [
        "cl1",
        "cl2",
        "cl3",
        "cl4",
    ]

    process_lines = [
        [
            exename, data1, data2, name
        ] + [','.join([
            x
            for x
            in names
            if x != name
        ])]
        for name
        in names
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


if __name__ == "__main__":
    main()
