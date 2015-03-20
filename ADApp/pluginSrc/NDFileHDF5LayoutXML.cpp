/*
 * NDFileHDF5LayoutXML.cpp
 *
 *  Created on: 26 Jan 2012
 *      Author: up45
 */

#include <cstdlib>
#include <algorithm>
#include <string>
#include <iostream>

#include <libxml/xmlreader.h>
#include "NDFileHDF5Layout.h"
#include "NDFileHDF5LayoutXML.h"

namespace hdf5
{

  const std::string LayoutXML::ATTR_ELEMENT_NAME           = "name";
  const std::string LayoutXML::ATTR_ROOT                   = "hdf5_layout";
  const std::string LayoutXML::ATTR_GROUP                  = "group";
  const std::string LayoutXML::ATTR_DATASET                = "dataset";
  const std::string LayoutXML::ATTR_ATTRIBUTE              = "attribute";
  const std::string LayoutXML::ATTR_GLOBAL                 = "global";
  const std::string LayoutXML::ATTR_HARDLINK               = "hardlink";

  const std::string LayoutXML::ATTR_ROOT_NDATTR_DEFAULT    = "auto_ndattr_default";
  const std::string LayoutXML::ATTR_SOURCE                 = "source";
  const std::string LayoutXML::ATTR_SRC_DETECTOR           = "detector";
  const std::string LayoutXML::ATTR_SRC_DET_DEFAULT        = "det_default";
  const std::string LayoutXML::ATTR_SRC_NDATTR             = "ndattribute";
  const std::string LayoutXML::ATTR_SRC_CONST              = "constant";
  const std::string LayoutXML::ATTR_SRC_CONST_VALUE        = "value";
  const std::string LayoutXML::ATTR_SRC_CONST_TYPE         = "type";
  const std::string LayoutXML::ATTR_GRP_NDATTR_DEFAULT     = "ndattr_default";
  const std::string LayoutXML::ATTR_SRC_WHEN               = "when";
  const std::string LayoutXML::ATTR_GLOBAL_NAME            = "name";
  const std::string LayoutXML::ATTR_GLOBAL_VALUE           = "ndattribute";
  const std::string LayoutXML::ATTR_HARDLINK_NAME          = "name";
  const std::string LayoutXML::ATTR_HARDLINK_TARGET        = "target";

  const std::string LayoutXML::DEFAULT_LAYOUT = " \
  <group name=\"entry\"> \
    <attribute name=\"NX_class\" source=\"constant\" value=\"NXentry\" type=\"string\"></attribute> \
    <group name=\"instrument\"> \
      <attribute name=\"NX_class\" source=\"constant\" value=\"NXinstrument\" type=\"string\"></attribute> \
      <group name=\"detector\"> \
        <attribute name=\"NX_class\" source=\"constant\" value=\"NXdetector\" type=\"string\"></attribute> \
        <dataset name=\"data\" source=\"detector\" det_default=\"true\"> \
          <attribute name=\"NX_class\" source=\"constant\" value=\"SDS\" type=\"string\"></attribute> \
          <attribute name=\"signal\" source=\"constant\" value=\"1\" type=\"int\"></attribute> \
        </dataset> \
        <group name=\"NDAttributes\"> \
          <attribute name=\"NX_class\" source=\"constant\" value=\"NXcollection\" type=\"string\"></attribute> \
          <dataset name=\"ColorMode\" source=\"ndattribute\" ndattribute=\"ColorMode\"> \
          </dataset> \
        </group>          <!-- end group NDAttribute --> \
      </group>            <!-- end group detector --> \
      <group name=\"NDAttributes\" ndattr_default=\"true\"> \
        <attribute name=\"NX_class\" source=\"constant\" value=\"NXcollection\" type=\"string\"></attribute> \
      </group>            <!-- end group NDAttribute (default) --> \
      <group name=\"performance\"> \
        <dataset name=\"timestamp\"></dataset> \
      </group>            <!-- end group performance --> \
    </group>              <!-- end group instrument --> \
    <group name=\"data\"> \
      <attribute name=\"NX_class\" source=\"constant\" value=\"NXdata\" type=\"string\"></attribute> \
      <hardlink name=\"data\" target=\"/entry/instrument/detector/data\"></hardlink> \
    </group>              <!-- end group data --> \
  </group>                <!-- end group entry --> ";

  LayoutXML::LayoutXML() : auto_ndattr_default(true), ptr_tree(NULL), ptr_curr_element(NULL)
  {
    log = log4cxx::Logger::getLogger("LayoutXML");

    /* Initialize the libxml2 library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used. */
    LIBXML_TEST_VERSION
    this->xmlreader = NULL;
  }

  LayoutXML::~LayoutXML()
  {
    if (this->ptr_tree){
      delete this->ptr_tree;
    }
  }

  int LayoutXML::load_xml()
  {
    int ret = 0;
    auto_ndattr_default = true;
    this->xmlreader = xmlReaderForMemory(DEFAULT_LAYOUT.c_str(),
                                         (int)DEFAULT_LAYOUT.size(),
                                         "noname.xml",
                                         NULL,
                                         0);

    if (this->xmlreader == NULL){
      LOG4CXX_ERROR(log, "Unable to open default XML string");
      return -1;
    }

    LOG4CXX_INFO(log, "Parsing default HDF5 layout XML string");
    while ( (ret = xmlTextReaderRead(this->xmlreader)) == 1){
      this->process_node();
    }
    xmlFreeTextReader(this->xmlreader);
    this->xmlreader = NULL;
    if (ret != 0){
      LOG4CXX_ERROR(log, "Failed to parse default XML string");
    }

    // If no elements were created then we've failed...
    if (this->ptr_tree == NULL){
      LOG4CXX_ERROR(log, "Unable to create the Root tree!");
      return -1;
    }

    LOG4CXX_DEBUG(log, "XML layout tree shape: " << this->ptr_tree->_str_());
    return ret;
  }

  int LayoutXML::load_xml(const std::string& filename)
  {
    int ret = 0;
    auto_ndattr_default = true;
    // if the file name contains <?xml then load it as an xml string from memory
    if (filename.find("<?xml") != std::string::npos){
      this->xmlreader = xmlReaderForMemory(filename.c_str(), (int)filename.length(), NULL, NULL, 0);
    } else {
      this->xmlreader = xmlReaderForFile(filename.c_str(), NULL, 0);
    }

    if (this->xmlreader == NULL){
      LOG4CXX_ERROR(log, "Unable to open XML file: " << filename );
      return -1;
    }

    LOG4CXX_INFO(log, "Loading HDF5 layout XML file: " << filename);
    while ((ret = xmlTextReaderRead(this->xmlreader)) == 1){
      this->process_node();
    }
    xmlFreeTextReader(this->xmlreader);
    this->xmlreader = NULL;
    if (ret != 0){
      LOG4CXX_ERROR(log, "Failed to parse XML file: "<< filename );
    }

    // If no elements were created then we've failed...
    if (this->ptr_tree == NULL){
      LOG4CXX_ERROR(log, "Unable to create the Root tree!" );
      return -1;
    }

    LOG4CXX_DEBUG(log, "XML layout tree shape: " << this->ptr_tree->_str_() );
    return ret;
  }

  int LayoutXML::verify_xml(const std::string& filename)
  {
    int ret = 0;
    int status = 0;
    auto_ndattr_default = true;

    // if the file name contains <?xml then load it as an xml string from memory
    if (filename.find("<?xml") != std::string::npos){
      this->xmlreader = xmlReaderForMemory(filename.c_str(), (int)filename.length(), NULL, NULL, 0);
      if (this->xmlreader == NULL){
        LOG4CXX_ERROR(log, "Unable to parse XML string: " << filename );
        this->xmlreader = NULL;
        status = -1;
      }
    } else {
      this->xmlreader = xmlReaderForFile(filename.c_str(), NULL, 0);
      if (this->xmlreader == NULL){
        LOG4CXX_ERROR(log, "Unable to open XML file: " << filename );
        this->xmlreader = NULL;
        status = -1;
      }
    }

    if  (status == 0){
      LOG4CXX_INFO(log, "Loading HDF5 layout XML file: " << filename);
      while ((ret = xmlTextReaderRead(this->xmlreader)) == 1) {
        this->process_node();
      }
      xmlFreeTextReader(this->xmlreader);
      this->xmlreader = NULL;
      if (ret != 0){
        LOG4CXX_ERROR(log, "Failed to parse XML file: "<< filename );
        status = -1;
      }
    }
    // Parsed OK, now free everything
    if (this->ptr_tree){
      delete this->ptr_tree;
    }
    this->ptr_tree = NULL;
    // Empty the globals store
    this->globals.clear();

    return status;
  }

  /**
   * Free all resources and reset the xml tree
   */
  int LayoutXML::unload_xml()
  {
    // Release any stored resource elements
    if (this->ptr_tree != NULL){
      delete this->ptr_tree;
      this->ptr_tree = NULL;
    }

    // Empty the globals store
    this->globals.clear();

    // Release the xml reader
    if (this->xmlreader != NULL){
      xmlFreeTextReader(this->xmlreader);
      this->xmlreader = NULL;
    }
    return 0;
  }

  Root* LayoutXML::get_hdftree()
  {
    return this->ptr_tree;
  }

  std::string LayoutXML::get_global(const std::string& name)
  {
    return this->globals[name];
  }

  bool LayoutXML::getAutoNDAttrDefault()
  {
    return auto_ndattr_default;
  }

  /** Process one XML node and create the necessary HDF5 element if necessary
   *
   */
  int LayoutXML::process_node()
  {
    xmlReaderTypes type = (xmlReaderTypes)xmlTextReaderNodeType(this->xmlreader);
    int ret = 0;
    const xmlChar* xmlname = NULL;

    xmlname = xmlTextReaderConstName(this->xmlreader);
    if (xmlname == NULL){
      return ret;
    }

    std::string name((const char*)xmlname);
    switch( type )
    {
      // Elements can be either 'group', 'dataset' or 'attribute'
      case XML_READER_TYPE_ELEMENT:
        //LOG4CXX_DEBUG(log, "process_node: \'" << name << "\' (" << xmlname << ")" );
        if (name == LayoutXML::ATTR_ROOT){
          ret = this->parse_root();
        } else if (name == LayoutXML::ATTR_GROUP){
          ret = this->new_group();
        } else if (name == LayoutXML::ATTR_DATASET){
          ret = this->new_dataset();
        } else if (name == LayoutXML::ATTR_ATTRIBUTE){
          ret = this->new_attribute();
        } else if (name == LayoutXML::ATTR_GLOBAL){
          ret = this->new_global();
        } else if (name == LayoutXML::ATTR_HARDLINK){
          ret = this->new_hardlink();
        }
        if (ret != 0){
          LOG4CXX_WARN(log, "adding new node: " << name << " failed..." );
        }
        break;

      // Parser callback at the end of an element.
      case XML_READER_TYPE_END_ELEMENT:
        if (this->ptr_tree == NULL){
          break;
        }
        if (name == LayoutXML::ATTR_GROUP || name == LayoutXML::ATTR_DATASET){
          //LOG4CXX_DEBUG(log, "END ELEMENT name: " << name << " curr: " << this->ptr_curr_element->get_full_name() );
          if (this->ptr_curr_element != NULL){
            this->ptr_curr_element = this->ptr_curr_element->get_parent();
          }
          if (this->ptr_curr_element == NULL){
            this->ptr_curr_element = this->ptr_tree;
          }
        }
        break;

      default:
        break;
    }

    // Check if this is an empty element.  If it is then execute as if it was the end of an element
    if (xmlTextReaderIsEmptyElement(this->xmlreader) && (this->ptr_tree != NULL)){
      if (name == LayoutXML::ATTR_GROUP || name == LayoutXML::ATTR_DATASET){
        //LOG4CXX_DEBUG(log, "END ELEMENT name: " << name << " curr: " << this->ptr_curr_element->get_full_name() );
        if (this->ptr_curr_element != NULL){
          this->ptr_curr_element = this->ptr_curr_element->get_parent();
        }
        if (this->ptr_curr_element == NULL){
          this->ptr_curr_element = this->ptr_tree;
        }
      }
    }
    return ret;
  }

  /** Process a datasets XML attributes
   * to work out the source of the attribute value: either detector data, NDAttribute or constant.
   */
  int LayoutXML::process_dset_xml_attribute(DataSource& out)
  {
    int ret = -1;
    if (! xmlTextReaderHasAttributes(this->xmlreader) ) return ret;

    xmlChar *attr_src = NULL;
    std::string str_attr_src;

    attr_src = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_SOURCE.c_str());
    if (attr_src == NULL) return ret;
    str_attr_src = (char*)attr_src;

    if (str_attr_src == LayoutXML::ATTR_SRC_DETECTOR){
      out = DataSource( detector );
      ret = 0;
    }
    if (str_attr_src == LayoutXML::ATTR_SRC_NDATTR){
      out = DataSource( ndattribute );
      ret = 0;
    }
    if (str_attr_src == LayoutXML::ATTR_SRC_CONST){
      out = DataSource( constant );
      ret = 0;
    }
    return ret;
  }

  int LayoutXML::process_attribute_xml_attribute(Attribute& out)
  {
    int ret = -1;
    if (! xmlTextReaderHasAttributes(this->xmlreader) ) return ret;

    xmlChar *attr_src = NULL;
    std::string str_attr_src;

    attr_src = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_SOURCE.c_str());
    if (attr_src == NULL) return ret;
    str_attr_src = (char*)attr_src;

    std::string str_attr_val = "";
    xmlChar *attr_val = NULL;
    std::string str_attr_type = "";
    xmlChar *attr_type = NULL;
    std::string str_attr_when = "";
    xmlChar *attr_when = NULL;

    // If the tag is: source="ndattribute"
    // Then setup the data source as a ndattribute with the name of the NDAttribute as argument value
    if (str_attr_src == LayoutXML::ATTR_SRC_NDATTR) {
      attr_val = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_SRC_NDATTR.c_str());
      if (attr_val != NULL) str_attr_val = (char*)attr_val;
      out.source = DataSource( ndattribute, str_attr_val );
      // Check for a when="OnFileClose" or when="OnFileOpen" tag
      attr_when = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_SRC_WHEN.c_str());
      if (attr_when != NULL) str_attr_when = (char*)attr_when;
      if (str_attr_when == "OnFileOpen"){
        out.source.set_when_to_save(OnFileOpen);
      } else if (str_attr_when == "OnFileClose"){
        out.source.set_when_to_save(OnFileClose);
      }
    }

    // On the other hand if source="constant"
    // Then setup the data source as a constant with a string value.
    // Todo: a constant currently is only supported as a string type. This is
    // probably Good Enough (TM) for most use but should really be made work for
    // at least a int and float as well.
    else if (str_attr_src == LayoutXML::ATTR_SRC_CONST) {
      attr_val = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_SRC_CONST_VALUE.c_str());
      if (attr_val != NULL) str_attr_val = (char*)attr_val;
      out.source = DataSource( constant, str_attr_val );
      attr_type = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_SRC_CONST_TYPE.c_str());
      if (attr_type != NULL) {
        str_attr_type = (char*)attr_type;
        DataType_t dtype = string;
        if (str_attr_type == "int") dtype = int32;
        else if (str_attr_type == "float") dtype = float64;
        else if (str_attr_type == "string") dtype = string;
        out.source.set_const_datatype_value(dtype, str_attr_val);
      }
    }
    ret = 0;
    return ret;
  }

  int LayoutXML::parse_root()
  {
    // First check the basics, no attributes is not an error but we should return
    if (! xmlTextReaderHasAttributes(this->xmlreader) ) return 0;

    // Check for the auto_ndattr_default tag
    xmlChar * root_auto = NULL;
    root_auto = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar *)LayoutXML::ATTR_ROOT_NDATTR_DEFAULT.c_str());

    // A tag has been added but it's not the one we expect, return an error
    if (root_auto == NULL) return -1;

    // Get the standard string representation of the tag
    std::string str_root_auto((char*)root_auto);
    xmlFree(root_auto);

    // Now check if we have the correct string
    if (str_root_auto == "false"){
      // Set our flag to false and return success
      auto_ndattr_default = false;
      return 0;
    } else if (str_root_auto == "true"){
      // Do nothing but return success
      return 0;
    }
    // Any strings other than false (or true) are not understood, return an error
    return -1;
  }

  int LayoutXML::new_group()
  {
    // First check the basics
    if (! xmlTextReaderHasAttributes(this->xmlreader) ) return -1;

    xmlChar * group_name = NULL;
    group_name = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar *)LayoutXML::ATTR_ELEMENT_NAME.c_str());
    //LOG4CXX_DEBUG(log, "  new_group: " << group_name );
    if (group_name == NULL) return -1;

    std::string str_group_name((char*)group_name);
    xmlFree(group_name);

    // Initialise the tree if it has not already been done.
    if (this->ptr_tree == NULL){
      this->ptr_tree = new Root();
      //LOG4CXX_DEBUG(log, "  Initialised the root of the tree: " << *this->ptr_tree );
      this->ptr_curr_element = this->ptr_tree;
    }

    // Obtain the parent and create this group from that parent
    Group *parent = static_cast<Group *>(this->ptr_curr_element);
    Group *new_group = NULL;
    new_group = parent->new_group(str_group_name);
    if (new_group == NULL) return -1;
    this->ptr_curr_element = new_group;

    if (this->ptr_curr_element != NULL){
      // check whether this group is tagged as the default group to place
      // NDAttribute meta-data in
      xmlChar * ndattr_default;
      ndattr_default = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar *)LayoutXML::ATTR_GRP_NDATTR_DEFAULT.c_str());
      if (ndattr_default != NULL){
        std::string str_ndattr_default((char*)ndattr_default);
        xmlFree(ndattr_default);
        // if the group has tag: ndattr_default="true" (true in lower case)
        // then set the group as the default container for NDAttributes.
        if (str_ndattr_default == "true"){
          Group * new_group = static_cast<Group*>(this->ptr_curr_element);
          new_group->set_default_ndattr_group();
        }
      }
    }
    return 0;
  }

  int LayoutXML::new_dataset()
  {
    // First check the basics
    if (! xmlTextReaderHasAttributes(this->xmlreader) ) return -1;
    if (this->ptr_curr_element == NULL) return -1;

    std::string str_val = "";
    xmlChar *c_val = NULL;
    std::string str_type = "";
    xmlChar *c_type = NULL;

    xmlChar *dset_name = NULL;
    dset_name = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar *)LayoutXML::ATTR_ELEMENT_NAME.c_str());
    //LOG4CXX_DEBUG(log, "  new_dataset: " << dset_name );
    if (dset_name == NULL) return -1;

    std::string str_dset_name((char*)dset_name);
    Group *parent = (Group *)this->ptr_curr_element;
    Dataset *dset = NULL;
    dset = parent->new_dset(str_dset_name);
    if (dset == NULL) return -1;

    // Check if this dataset has been set as the default
    bool detector_default = false;
    xmlChar *attr_def = NULL;
    std::string str_attr_def;
    attr_def = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_SRC_DET_DEFAULT.c_str());
    if (attr_def != NULL){
      str_attr_def = (char*)attr_def;
      if (str_attr_def == "true"){
        detector_default = true;
      }
    }
    dset->set_src_default(detector_default);

    this->ptr_curr_element = dset;

    DataSource attrval;
    this->process_dset_xml_attribute(attrval);   

    // Check for ndattribute and store the name
    if (attrval.is_src_ndattribute()){
      xmlChar *attr_ndname = NULL;
      std::string str_attr_ndname;
      attr_ndname = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_SRC_NDATTR.c_str());
      if (attr_ndname != NULL){
        str_attr_ndname = (char*)attr_ndname;
        dset->set_ndattr_name(str_attr_ndname);
        //if ndattribute, check for 'when' tag
        xmlChar *attr_when = NULL;
        attr_when = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_SRC_WHEN.c_str());
        if (attr_when != NULL){
          std::string str_attr_when( (char*)attr_when );
          When_t when_to_save = OnFrame; //Default is to save every frame
          if (str_attr_when == "OnFileOpen"){
            when_to_save = OnFileOpen;
          } else if(str_attr_when == "OnFileClose"){
            when_to_save = OnFileClose;
          }
          attrval.set_when_to_save(when_to_save);
        }
      }
    } else if (attrval.is_src_constant()){
      //if constant data
      c_val = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_SRC_CONST_VALUE.c_str());
      if (c_val != NULL) str_val = (char*)c_val;
      c_type = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_SRC_CONST_TYPE.c_str());
      if (c_type != NULL){
        str_type = (char*)c_type;
        DataType_t dtype = string;
        if (str_type == "int") dtype = int32;
        else if (str_type == "float") dtype = float64;
        else if (str_type == "string") dtype = string;
        attrval.set_const_datatype_value(dtype, str_val);
      }
    }
    dset->set_data_source(attrval);
    return 0;
  }

  int LayoutXML::new_attribute()
  {
    int ret = 0;
    // First check the basics
    if (! xmlTextReaderHasAttributes(this->xmlreader) ) return -1;
    if (this->ptr_curr_element == NULL) return -1;

    xmlChar *ndattr_name = NULL;
    ndattr_name = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_ELEMENT_NAME.c_str());
    //LOG4CXX_DEBUG(log, "  new_attribute: " << ndattr_name << " attached to: " << this->ptr_curr_element->get_full_name() );
    if (ndattr_name == NULL) return -1;

    std::string str_ndattr_name((char*)ndattr_name);
    Attribute ndattr(str_ndattr_name);
    this->process_attribute_xml_attribute(ndattr);

    ret = this->ptr_curr_element->add_attribute(ndattr);
    return ret;
  }

  int LayoutXML::new_global()
  {
    int ret = 0;
    // First check the basics
    if (! xmlTextReaderHasAttributes(this->xmlreader) ) return -1;
    xmlChar *global_name = NULL;
    global_name = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_GLOBAL_NAME.c_str());
    if (global_name == NULL) return -1;
    xmlChar *global_value = NULL;
    global_value = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_GLOBAL_VALUE.c_str());
    if (global_value == NULL) return -1;

    std::string str_global_name((char*)global_name);
    std::string str_global_value((char*)global_value);

    this->globals[str_global_name] = str_global_value;
    return ret;
  }

  int LayoutXML::new_hardlink()
  {
    // First check the basics
    if (! xmlTextReaderHasAttributes(this->xmlreader) ) return -1;
    xmlChar *hardlink_name = NULL;
    hardlink_name = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_ELEMENT_NAME.c_str());
    if (hardlink_name == NULL) return -1;
    xmlChar *hardlink_target = NULL;
    hardlink_target = xmlTextReaderGetAttribute(this->xmlreader, (const xmlChar*)LayoutXML::ATTR_HARDLINK_TARGET.c_str());
    if (hardlink_target == NULL) return -1;

    std::string str_hardlink_name((char*)hardlink_name);
    Group *parent = (Group *)this->ptr_curr_element;
    HardLink *hardlink = NULL;
    hardlink = parent->new_hardlink(str_hardlink_name);
    if (hardlink == NULL) return -1;
    const std::string str_hardlink_target((char*)hardlink_target);
    hardlink->set_target(str_hardlink_target);
    return 0;
  }

} // hdf5



