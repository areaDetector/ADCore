Plugin Tests
============

This folder contains unit tests for Area Detector plugins.

So far only test_NDPluginCircularBuffer.cpp exists.

Building
--------

The plugin test target is not built by default since it uses the Boost Unit Testing Framework. To build it, 
add the line

    USE_BOOST_UTF = YES

to your CONFIG_SITE.local file.

Running
-------

The plugin test binary is installed at the path 

    /bin/linux-x86_64/NDPluginCircularBuff_test

Running this binary from the command line should run the test suite and provide a summary indicating 
which tests, if any, failed.