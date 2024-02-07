Getting started
===============

1. Build
--------

Luci comes with an interpreter.  To build it download the code from location below.  Then use cmake to produce makefiles.

.. code:: bash

    git clone https://github.com/nightwing1978/luci-lang

Building a debug build using 10 threads, navigate to the main directory and:

.. code:: bash

    cmake --build build --config Debug --target ALL_BUILD -j 10

After the build has finished the tests can be ran:

.. code:: bash

    cd tests
    ../build/interp/Debug/luci.exe run_tests.luci


2. REPL environment
-------------------

Luci comes with an interpreter.  It can be used in REPL mode or can be used to execute a given .luci script and then exit.  Starting the interpreter is done by:

.. code:: bash

    ../build/interp/Debug/luci.exe

Exiting the interactive interpreter is done by entering the `exit()` function. The interpreter does have a number of options, they can be requested by:

.. code:: bash

    ../build/interp/Debug/luci.exe --help






