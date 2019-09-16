#!/usr/bin/env python3

import subprocess


def main():
    exename = "./cmake-build-release/libs/ml/examples/word2vec_multiprocess_distributed_learning"
    data = "/home/jiri/workspace/sampletext/text8dir/text8"

    names = [
        "cl1",
        "cl2",
        "cl3",
        "cl4",
    ]

    process_lines = [
        [
            exename, data, name
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
