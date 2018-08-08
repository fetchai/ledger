# Ledger Documentation

## Prerequisites

The following python packages will need to be installed on the system in order to generate the documentation:

    pip3 install Sphinx sphinxcontrib-mermaid watchdog

## Generating the Documentation

Once sphinx and its dependencies are install simply run the following command:

	./generate.py

When developing the code it is recommended that you use the `--watch` flag of the generator script. This is automatically detect file changes and regenerate the documentation.

    ./generate.py -w

Or

    ./generate.py --watch
