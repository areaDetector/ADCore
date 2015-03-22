/** plugin-test.cpp
 * 
 *  This file just defines the basic level of the boost unittest
 *  systme for plugins. By doing this here, the actual unittests
 *  can span multiple source files.
 *
 *  Author: Ulrik Kofoed Pedersen, Diamond Light Source.
 *          20. March 2015
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "NDPlugin Tests"
#include <boost/test/unit_test.hpp>

