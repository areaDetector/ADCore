/*
 * NDFileHDF5Layout.h
 *
 *  Created on: 23 Jan 2012
 *      Author: up45
 */

#ifndef NDFILEHDF5LAYOUT_H_
#define NDFILEHDF5LAYOUT_H_

#include <ostream>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace hdf5 
{

  typedef enum {
    OnFrame,
    OnFileOpen,
    OnFileClose
  } When_t;

  typedef enum {
    notset,
    detector,
    ndattribute,
    constant
  } DataSrc_t;

  typedef enum {
    int8,
    uint8,
    int16,
    uint16,
    int32,
    uint32,
    float32,
    float64,
    string
  } DataType_t;

  /** Class used for writing a DataSource with the NDFileHDF5 plugin.
    */
  class DataSource
  {
    public:
      // Default constructor
      DataSource();
      // Initialising constructor
      DataSource(DataSrc_t src, const std::string& val);
      DataSource(DataSrc_t src);
      DataSource(DataSrc_t src, DataType_t type);
      // Copy constructor
      DataSource( const DataSource& src);
      // Assignment operator
      DataSource& operator=(const DataSource& src);
      ~DataSource(){};
      void set_datatype(DataType_t type);
      bool is_src_detector();
      bool is_src_ndattribute();
      bool is_src_constant();
      bool is_src(DataSrc_t src);

      std::string get_src_def(); /** return the string that define the source: either name of NDAttribute or constant value */
      DataType_t get_datatype();
      size_t datatype_size();
      void set_const_datatype_value(DataType_t dtype, const std::string& str_val);

      void set_when_to_save(When_t when);
      When_t get_when_to_save();
   
    private:
      DataSrc_t data_src;
      std::string val;
      DataType_t datatype;
      When_t when_to_save;
  };


  /** Class used for writing an Attribute with the NDFileHDF5 plugin.
    */
  class Attribute
  {
    public:
      Attribute(){};
      Attribute(const Attribute& src); // Copy constructor
      Attribute(std::string& name);
      Attribute(const char* name);
      Attribute(const char* name, DataSource& src);
      ~Attribute(){};
      Attribute& operator=(const Attribute& src);
      std::string get_name();
      void setOnFileOpen(bool onOpen);
      bool is_onFileOpen();
      bool is_onFileClose();

      DataSource source;
    private:
      void _copy(const Attribute& src);
      std::string name;
      bool onFileOpen;
  };

  /**
   * Describe a generic structure element with the NDFileHDF5 plugin.
   * An element can contain a number of attributes and
   * be a subset of a number of parents.
   */
  class Element
  {
    public:
      Element();
      Element(const Element& src);
      Element(const std::string& name);
      ~Element(){};
      Element& operator=(const Element& src);

      const std::string& get_name();
      virtual std::string get_full_name();
      virtual std::string get_path(bool trailing_slash=false);
      int add_attribute(Attribute& attr);
      bool has_attribute(const std::string& attr_name);
      int tree_level();
      Element *get_parent();
      typedef std::map<std::string, Attribute> MapAttributes_t;
      MapAttributes_t& get_attributes();

    protected:
      void _copy(const Element& src);
      MapAttributes_t attributes;
      std::string name;

    public:
      friend class Group;

    private:
      Element *parent;
  };


  /** Class used for writing a DataSet with the NDFileHDF5 plugin.
    */
  class Dataset: public Element
  {
    public:
      Dataset();
      Dataset(const std::string& name);
      Dataset(const Dataset& src);
      Dataset& operator=(const Dataset& src);
      virtual ~Dataset();

      /** Stream operator: use to prints a string representation of this class */
      inline friend std::ostream& operator<<(std::ostream& out, Dataset& dset)
      { out << dset._str_(); return out; }
      std::string _str_();  /** Return a string representation of the object */

      void set_data_source(DataSource& src);
      void set_data_source(DataSource& src, size_t max_elements);
      DataSource& data_source();

      void data_alloc_max_elements(size_t max_elements);
      size_t data_append_value(void * val);
      size_t data_num_elements();
      size_t data_store_size();
      void data_stored();
      const void * data();
      void set_src_default(bool def);
      bool is_src_default();
      void set_ndattr_name(const std::string& name);
      bool has_ndattr_name(const std::string& name);

    private:
      void _copy(const Dataset& src);
      DataSource datasource;

      void data_clear();
      void * data_ptr;
      size_t data_nelements;
      size_t data_current_element;
      size_t data_max_bytes;
      size_t data_nelements_stored;
      bool data_default;
      std::string ndattr_name;
  };

  /** Class used for writing a HardLink with the NDFileHDF5 plugin.
    */
  class HardLink: public Element
  {
    public:
      HardLink();
      HardLink(const std::string& name);
      HardLink(const HardLink& src);
      HardLink& operator=(const HardLink& src);
      virtual ~HardLink();

      /** Stream operator: use to prints a string representation of this class */
      inline friend std::ostream& operator<<(std::ostream& out, HardLink& hardLink)
      { out << hardLink._str_(); return out; }
      std::string _str_();  /** Return a string representation of the object */

      void set_target(const std::string& target);
      std::string& get_target();

    private:
      void _copy(const HardLink& src);
      std::string target;
  };

  /**
   * Describe a group element.
   * A group is like a directory in a file system. It can contain
   * other groups and datasets (like files).
   */
  class Group: public Element
  {
    public:
      Group();
      Group(const std::string& name);
      Group(const char * name);
      Group(const Group& src);
      virtual ~Group();
      Group& operator=(const Group& src);

      Dataset* new_dset(const std::string& name);
      Dataset* new_dset(const char * name);
      Group* new_group(const std::string& name);
      Group* new_group(const char * name);
      HardLink* new_hardlink(const std::string& name);
      HardLink* new_hardlink(const char * name);
      int find_dset_ndattr(const std::string& ndattr_name, Dataset** dset); /** << Find and return a reference to the dataset for a given NDAttribute */
      int find_dset_ndattr(const char * ndattr_name, Dataset** dset);
      int find_dset( std::string& dsetname, Dataset** dest);
      int find_dset( const char* dsetname, Dataset** dest);
      void set_default_ndattr_group();
      Group* find_ndattr_default_group(); /** << search through subgroups to return a pointer to the NDAttribute default container group */
      int find_detector_default_dset(Dataset** dset);
      int num_groups();
      int num_datasets();

      /** Stream operator: use to prints a string representation of this class */
      inline friend std::ostream& operator<<(std::ostream& out, Group& grp)
      { out << grp._str_(); return out; }
      std::string _str_();  /** Return a string representation of the object */

      typedef std::map<std::string, Group*> MapGroups_t;
      typedef std::map<std::string, Dataset*> MapDatasets_t;
      typedef std::map<std::string, HardLink*> MapHardLinks_t;
      MapGroups_t& get_groups();
      MapDatasets_t& get_datasets();
      MapHardLinks_t& get_hardlinks();
      void find_dsets(DataSrc_t source, MapDatasets_t& dsets); /** return a map of datasets [string name, Dataset dset] which contains all datasets, marked as [source] data. */

      typedef std::map<std::string, DataSource*> MapNDAttrSrc_t;
      virtual void merge_ndattributes(MapNDAttrSrc_t::const_iterator it_begin,
                                      MapNDAttrSrc_t::const_iterator it_end,
                                      std::set<std::string>& used_ndattribute_srcs);

    private:
      void _copy(const Group& src);
      bool name_exist(const std::string& name);
      bool ndattr_default_container;
      std::map<std::string, Dataset*> datasets;
      std::map<std::string, Group*> groups;
      std::map<std::string, HardLink*> hardlinks;
  };

  /** Class used for writing the root of the file with the NDFileHDF5 plugin.
    */
  class Root: public Group 
  {
    public:
      Root();
      Root(const std::string& name);
      Root(const char *name);
      virtual ~Root(){};
      virtual void merge_ndattributes(MapNDAttrSrc_t::const_iterator it_begin,
                                      MapNDAttrSrc_t::const_iterator it_end,
                                      std::set<std::string>& used_ndattribute_srcs);
      std::string get_full_name(){ return ""; };
      std::string get_path(bool trailing_slash=false){ return ""; };
  };

} // hdf5


#endif /* LAYOUT_H_ */
