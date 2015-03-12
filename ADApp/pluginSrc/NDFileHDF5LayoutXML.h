/*
 * NDFileHDF5LayoutXML.h
 *
 *  Created on: 26 Jan 2012
 *      Author: up45
 */

#ifndef NDFILEHDF5LAYOUTXML_H_
#define NDFILEHDF5LAYOUTXML_H_

#include <libxml/xmlreader.h>
#include <string>
#include <map>
#ifdef LOG4CXX
#include <log4cxx/logger.h>
#else

// std::cout logging if log4cxx is not defined
// define HDF5_LOGGING 1 to print log messages
//#define HDF5_LOGGING 1

namespace log4cxx
{
  class Logger
  {
    public:
      static Logger* getLogger(const std::string& name){ return new Logger(); }
  };
  typedef Logger* LoggerPtr;
} // namespace log4cxx

#ifdef HDF5_LOGGING
  #define LOG4CXX_ERROR(LOG, msg) std::cout << msg << std::endl
  #define LOG4CXX_INFO(LOG, msg) std::cout << msg << std::endl
  #define LOG4CXX_DEBUG(LOG, msg) std::cout << msg << std::endl
  #define LOG4CXX_WARN(LOG, msg) std::cout << msg << std::endl
#else
  #define LOG4CXX_ERROR(LOG, msg)
  #define LOG4CXX_INFO(LOG, msg)
  #define LOG4CXX_DEBUG(LOG, msg)
  #define LOG4CXX_WARN(LOG, msg)
#endif
#endif

namespace hdf5
{

  // forward declarations
  class Group;
  class Root;
  class DataSource;
  class Attribute;
  class Element;

  int main_xml(const char *fname);

  /**  Used to define layout of HDF5 file with NDFileHDF5 plugin 
    */ 
  class LayoutXML
  {
    public:
      static const std::string ATTR_ELEMENT_NAME;
      static const std::string ATTR_ROOT;
      static const std::string ATTR_GROUP;
      static const std::string ATTR_DATASET;
      static const std::string ATTR_ATTRIBUTE;
      static const std::string ATTR_GLOBAL;
      static const std::string ATTR_HARDLINK;

      static const std::string ATTR_ROOT_NDATTR_DEFAULT;
      static const std::string ATTR_SOURCE;
      static const std::string ATTR_SRC_DETECTOR;
      static const std::string ATTR_SRC_DET_DEFAULT;
      static const std::string ATTR_SRC_NDATTR;
      static const std::string ATTR_SRC_CONST;
      static const std::string ATTR_SRC_CONST_VALUE;
      static const std::string ATTR_SRC_CONST_TYPE;
      static const std::string ATTR_GRP_NDATTR_DEFAULT;
      static const std::string ATTR_SRC_WHEN;
      static const std::string ATTR_GLOBAL_NAME;
      static const std::string ATTR_GLOBAL_VALUE;
      static const std::string ATTR_HARDLINK_NAME;
      static const std::string ATTR_HARDLINK_TARGET;

      static const std::string DEFAULT_LAYOUT;

      LayoutXML();
      ~LayoutXML();

      int load_xml();
      int load_xml(const std::string& filename);
      int verify_xml(const std::string& filename);
      int unload_xml();

      Root* get_hdftree();
      std::string get_global(const std::string& name);
      bool getAutoNDAttrDefault();

    private:
      int process_node();

      int process_dset_xml_attribute(DataSource& out);
      int process_attribute_xml_attribute(Attribute& out);

      int parse_root();
      int new_group();
      int new_dataset();
      int new_attribute();
      int new_global();
      int new_hardlink();

      bool auto_ndattr_default;
      log4cxx::LoggerPtr log;
      Root* ptr_tree;
      Element *ptr_curr_element;
      xmlTextReaderPtr xmlreader;
      std::map<std::string, std::string> globals;
  };

} // hdf5

#endif /* NDFILEHDF5LAYOUTXML_H_ */

