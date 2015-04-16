Plugin Tests
============

This folder contains unit tests for Area Detector plugins.

So far it includes tests for:

* The CircularBuffer plugin
* The HDF5 file writer plugin (although incomplete)

Building
--------

The plugin test target is not built by default since it has an external dependency
on the Boost Unit Testing Framework. To build it make sure the following macros
are defined in the build system (for example in CONFIG_SITE.<arch>.Common):

    BOOST=/usr
    BOOST_LIB=$(BOOST)/lib64
    BOOST_INCLUDE=$(BOOST)/include


If the "BOOST" macro is defined the tests will be built.

Running
-------

The plugin test binary is installed in the bin dir with the name "plugin-test".

See available options with the --help flag.

Can be run like this:

    ./bin/linux-x86_64/plugin-test --log_level=test_suite
    
Which results in a trace like this (example with forced failure):

    Running 4 test cases...
    Entering test suite "NDPlugin Tests"
    Entering test suite "NDFileHDF5Tests"
    Entering test case "test_Capture"
    This is just a message...
    ../test_NDFileHDF5.cpp(78): error in "test_Capture": check 0 == 1 failed [0 != 1]
    Leaving test case "test_Capture"
    Leaving test suite "NDFileHDF5Tests"
    Entering test suite "CircularBuffTests"
    Entering test case "test_BufferWrappingAndStatusMessages"
    Leaving test case "test_BufferWrappingAndStatusMessages"
    Entering test case "test_OutputCount"
    Leaving test case "test_OutputCount"
    Entering test case "test_PreBufferOrder"
    Leaving test case "test_PreBufferOrder"
    Leaving test suite "CircularBuffTests"
    Leaving test suite "NDPlugin Tests"
    
    *** 1 failure detected in test suite "NDPlugin Tests"

Adding more tests
-----------------

The intention is that this framework will allow adding more tests in a simple
fashion. It is recommended to add tests while developing a new plugin for example.

The test framework uses the [Boost Test Library](http://www.boost.org/doc/libs/1_57_0/libs/test/doc/html/index.html)
as a unit-testing framework and test runner.

Use the [Boost testing tools](http://www.boost.org/doc/libs/1_57_0/libs/test/doc/html/utf/testing-tools.html)
to do appropriate checks - and use also BOOST_[MESSAGE|WARN] macros to add
additional logging output related to the testing rather than using printf and
cout. asynPrint are still used internally in plugins and drivers - and can be 
configured for example in the test fixture setup.

If creating a new test, add the tests in a similar fashion to the existing ones:

Create a test source file: pluginTests/test_<PluginName>.cpp and add it to the
pluginTest/Makefile as indicated by the comments there.

A minimal test for the imaginary new plugin "NDPluginAmazingStuff" simply need to 
contain the following:

    #include <NDPluginAmazingStuff.h>
    #include "boost/test/unit_test.hpp"
    BOOST_FIXTURE_TEST_SUITE(NDPluginAmazingStuffTests, NDPluginAmazingStuffTestFixture)
    
    struct NDPluginAmazingStuffTestFixture
    {
      NDFileHDF5 *dut;                     // device under test
      asynInt32Client *enableCallbacks;    // Asyn parameters to manipulate during test
      asynInt32Client *blockingCallbacks;
    
        // Test setup - gets run before starting each test case
        NDPluginAmazingStuffTestFixture()
        {
          // Asyn manager doesn't like it if we try to reuse the same port name for multiple drivers (even if only one is ever instantiated at once), so
          // change it slightly for each test case.
          std::string dutport("dutport");
          uniqueAsynPortName(dutport);

          // This is the plugin under test
          dut = new NDPluginAmazingStuff(dutport.c_str(), 50, 1, "", 0, 0, 0);

          enableCallbacks = new asynInt32Client(testport.c_str(), 0, NDPluginDriverEnableCallbacksString);
          blockingCallbacks = new asynInt32Client(testport.c_str(), 0, NDPluginDriverBlockingCallbacksString);


          // Set the downstream plugin to receive callbacks from the test plugin and to run in blocking mode, so we don't need to worry about synchronisation
          // with the downstream plugin.
          enableCallbacks->write(1);
          blockingCallbacks->write(1);
        }
        
      // Test tear down - gets run after completing each test case
      ~NDPluginAmazingStuffTestFixture()
      {
        delete blockingCallbacks;
        delete enableCallbacks;
        delete dut;
      }
    };
    
    // Our test case, testing the important "functionality"
    BOOST_AUTO_TEST_CASE(test_functionality)
    {
    
       // Create some test data
      size_t dims[2] = {2,5};
      NDArray *testArray = arrayPool->alloc(2,dims,NDFloat64,0,NULL);
       
      // Pass test data through plugin
      dut->processCallbacks(pArray);
      
      // Do some checks with the boost test tools:
      BOOST_CHECK_EQUAL( 4, 4 ); // "check" will carry on testing even if it fails
      BOOST_REQUIRE_EQUAL(5,5);  // "require" will abort the rest of the test case if it fails
      BOOST_MESSAGE("This is my amazing test!") // Use this macro rather than printf or cout!!!
    }
    
    
    BOOST_AUTO_TEST_SUITE_END()
    
 
Unit testing of external plugins
-------------------------------- 

For "external" plugins (i.e. plugins that do not reside directly in the ADCore
repository) there are some utilities to help in the ADTestUtility library.
 
