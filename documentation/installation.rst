===================
Installation Guide
===================

Requirements
------------

Operating Systems
^^^^^^^^^^^^^^^^^

Psyllid can be installed on Linux and macOS systems.  Some functionality (e.g. the Fast Packet Acquisition) is only available on Linux.

Dependencies
^^^^^^^^^^^^^

* CMake 3.1 or higher
* C++11 (gcc 4.9 or higher; or clang 3 or higher)
* Boost 1.48 or higher
* HDF5
* rabbitmqc

These can all be installed from package managers (recommended) or by source.

Submodules
^^^^^^^^^^^

* midge
* dripline-cpp
* monarch

Basic Installation
------------------

1. Ensure the dependencies are installed.
2. Clone::

    > git clone https://github.com/project8/psyllid.git

or::

    > git clone git@github.com:project8/psyllid.git

3. Updated submodules::

    > git submodule update --init --recursive

4. Create a build directory and run CMake::

    > cd psyllid
    > mkdir build
    > cd build
    > cmake ..

5. Build and install (will install within the build directory)::

    > make install

6. Test the installation

    > bin/psyllid

With the dependencies installed clone into ``https://github.com/project8/psyllid.git``.
Make sure to also pull the submodules by navigating to the cloned directory and running:

``git submodule update --init --recursive``

Commonly Used Build Options
---------------------------

- ``CMAKE_INSTALL_PREFIX``: the default is the build directory; change to wherever you want your libraries and binaries installed
- ``CMAKE_BUILD_TYPE``: the default is ``DEBUG``; set to ``RELEASE`` for the fastest performance and less verbosity
- ``Psyllid_ENABLE_ITERATOR_TIMING``: the default is ``OFF``; set to ``ON`` to get some diagnostics on how fast the nodes are processing data
- ``Psyllid_ENABLE_TESTING``: the default is ``OFF``; set to ``ON`` to build the test programs
