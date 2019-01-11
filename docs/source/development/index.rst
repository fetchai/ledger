Developing for the ledger
=========================

The fetch ledger project uses a series of tools to help maintain the codebase. In particular the ``clang-format`` and ``clang-tidy`` tools.

Installing
----------

As discussed you will need to have the ``clang-format`` and ``clang-tidy`` executables install on your system before attempting to use any of the helper scripts provided

Ubuntu 18.04
~~~~~~~~~~~~

Under ubuntu these tools can simply install the package from ``apt`` as shown below:

.. code:: bash

	$ sudo apt install clang-format clang-tidy


CI Docker Image
~~~~~~~~~~~~~~~

If you are unable to install these tools on your system, it is recommended that you the CI docker image. Starting a shell inside this image (which is based on Ubuntu 18.04) can simply be done with the following command:

.. code:: bash

	$ ./run-d.sh bash


Code Formatting
---------------

The ledger project uses ``clang-format`` in order to automatically format code in the project. To apply these formatting changes to the project simply execute the following script from the root of the project:

.. code:: bash

	$ ./scripts/apply_style

This might take a while to process since it is make code changes to the project. If the developer only wants to see the changes that would have been made. They can run the following command:

.. code:: bash

	$ ./scripts/apply_style -w -a

Full documentation for this script can be found by running the following command:

.. code:: bash

	$ ./scripts/apply_style -w -a

Static Analysis
---------------

THe ledger project also uses the ``clang-tidy`` tool to run a series of syntax and static analysis based checks. This is far more comprehensive in comparison to the `clang-format` tool.As such it needs to be aware how the source was build.  Specifically it is looking for a ``compile_commands.json`` which is omited as part of the build process.

In order to run this tool make sure that you have build all targets in the project:

.. code:: bash

	$ make -j

For more information on how to build the targets please refer to the :doc:`../getting_started/index` guide. Once this is complete you can run the static anaylsis script:

.. code:: bash

	$ ./scripts/run_static_analysis.py <path to cmake build dir>

This command will then scan through the project and warn about potential issues. It is important to note that by default this script will not apply any of the suggested fixs.

If the developer would like `clang-tidy` to apply all the changes then the following should be run:

.. code:: bash

	$ ./scripts/run_static_analysis.py <path to cmake build dir> --fix

.. warning:: It should be noted that the automatic updates provided by ``clang-tidy`` are seldom applied 100% cleanly. To the point that the project might not compile afterwards. Developers must be aware of this and carefully check the changed generated manually.


