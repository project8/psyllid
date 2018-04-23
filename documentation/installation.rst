===================
Installation Guide
===================

Installation
-------------


Dependencies
^^^^^^^^^^^^^

* CMake 3.1 or higher
* C++11
* Boost 1.48 or higher
* HDF5
* rabbitmqc

Submodules
^^^^^^^^^^^

* midge
* dripline-cpp
* monarch

Installation guide
^^^^^^^^^^^^^^^^^^^

With the dependencies installed clone into ``https://github.com/project8/psyllid.git``.
Make sure to also pull the submodules by navigating to the cloned directory and running:

``git submodule update --init --recursive``

Create a *build* directory and run ``ccmake [path to psyllid]``.
If you are going to run psyllid with a node configuration including the *packet_receiver_fpa* it is recommended to set the *CMAKE_BUILD_TYPE* to *RELEASE*.
To build and install psyllid run ``cmake [path to psyllid]`` and ``make install`` in the *build* directory.