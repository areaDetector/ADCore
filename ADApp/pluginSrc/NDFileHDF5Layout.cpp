/*
 * layout.cpp
 *
 *  Created on: 23 Jan 2012
 *      Author: up45
 */

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstring>

#include "NDFileHDF5Layout.h"

namespace hdf5 
{

  /* ================== Attribute Class public methods ==================== */
  Attribute::Attribute(const Attribute& src)
  {
    this->_copy(src);
  }

  Attribute::Attribute(std::string& name) : name(name), onFileOpen(true)
  {
  }

  Attribute::Attribute(const char* name) : name(name), onFileOpen(true)
  {
  }

  Attribute::Attribute(const char* name, DataSource& src) : source(src), name(name), onFileOpen(true)
  {
  }


  Attribute& Attribute::operator=(const Attribute& src)
  {
    this->_copy(src);
    return *this;
  }

  std::string Attribute::get_name()
  {
    return this->name;
  }

  void Attribute::setOnFileOpen(bool onOpen)
  {
    this->onFileOpen = onOpen;
  }

  bool Attribute::is_onFileOpen()
  {
    When_t when = source.get_when_to_save();
    return (when == OnFileOpen) || (when == OnFrame);
  }

  bool Attribute::is_onFileClose()
  {
    When_t when = source.get_when_to_save();
    return (when == OnFileClose) || (when == OnFrame);
  }

  void Attribute::_copy(const Attribute& src)
  {
    this->name = src.name;
    this->source = src.source;
    this->onFileOpen = src.onFileOpen;
  }

  DataSource::DataSource() : data_src(notset), val(""), datatype(int8), when_to_save(OnFrame)
  {
  }

  DataSource::DataSource(DataSrc_t srctype, const std::string& val) : data_src(srctype), val(val), datatype(string), when_to_save(OnFrame)
  {
  }

  DataSource::DataSource(DataSrc_t src, DataType_t type) : data_src(src), val(""), datatype(type), when_to_save(OnFrame)
  {
  }

  DataSource::DataSource(DataSrc_t src) : data_src(src), val(""), datatype(string), when_to_save(OnFrame)
  {
  }

  DataSource::DataSource(const DataSource& src) : data_src(src.data_src), val(src.val), datatype(src.datatype), when_to_save(src.when_to_save)
  {
  }

  /**
   * Assignment operator
   * Copies the sources private data members to this object.
   */
  DataSource& DataSource::operator=(const DataSource& src)
  {
    this->data_src = src.data_src;
    this->val = src.val;
    this->datatype = src.datatype;
    this->when_to_save = src.when_to_save;
    return *this;
  };

  void DataSource::set_datatype(DataType_t type)
  {
	  this->datatype = type;
  }

  bool DataSource::is_src_constant()
  {
    return this->data_src == constant ? true : false;
  }

  bool DataSource::is_src_detector()
  {
    return this->data_src == detector ? true : false;
  }

  bool DataSource::is_src_ndattribute()
  {
    return this->data_src == ndattribute ? true : false;
  }

  bool DataSource::is_src(DataSrc_t src)
  {
    return this->data_src == src ? true : false;
  }

  std::string DataSource::get_src_def()
  {
    return this->val;
  }

  DataType_t DataSource::get_datatype()
  {
    return this->datatype;
  }

  size_t DataSource::datatype_size()
  {
    size_t retval = sizeof(char);
    switch (this->datatype)
    {
      case uint8:
      case int8:
        retval = 1;
        break;
      case uint16:
      case int16:
        retval = 2;
        break;
      case uint32:
      case int32:
      case float32:
        retval = 4;
        break;
      case float64:
        retval = 8;
        break;
      case string:
        retval = 1;
        break;
      default:
        retval = 0;
        break;
    }
    return retval;
  }

  void DataSource::set_const_datatype_value(DataType_t dtype, const std::string& str_val)
  {
    if (this->data_src != constant) return;
    this->set_datatype(dtype);
    this->val = str_val;
  }

  void DataSource::set_when_to_save(When_t when)
  {
    when_to_save = when;
  }

  When_t DataSource::get_when_to_save()
  {
    return when_to_save;
  }

  /* ================== Element Class public methods ==================== */
  Element::Element()
  {
    this->name = "";
    this->parent = NULL;
  }

  Element::Element(const Element& src)
  {
    this->_copy(src);
  }

  Element::Element(const std::string& name)
  {
    this->name = name;
    this->parent = NULL;
  }

  std::string Element::get_full_name()
  {
    std::string fname = this->get_path(true);
    fname += this->name;
    return fname;
  }

  std::string Element::get_path(bool trailing_slash)
  {
    std::string path;
    path.append("/");
    if (this->parent != NULL){
      path.insert(0, this->parent->get_name());
      path.insert(0, this->parent->get_path(true));
      if (! trailing_slash) path.erase(path.end() - 1);
    }
    return path;
  }

  Element& Element::operator=(const Element& src)
  {
    // Check for self-assignment
    if (this == &src){
      return *this;
    }
    this->_copy(src);
    return *this;
  }

  const std::string& Element::get_name()
  {
    return this->name;
  }

  Element * Element::get_parent()
  {
    return this->parent;
  }

  Element::MapAttributes_t& Element::get_attributes()
  {
    return this->attributes;
  }

  int Element::add_attribute(Attribute& attr)
  {
    if (this->attributes.count(attr.get_name()) != 0) return -1;
    std::pair<std::map<std::string, Attribute>::iterator,bool> ret;
    ret = this->attributes.insert(std::pair<std::string, Attribute>(attr.get_name(), attr));
    // Check for successful insertion.
    if (ret.second == false) return -1;
    return 0;
  }

  bool Element::has_attribute(const std::string& attr_name)
  {
    return this->attributes.count(attr_name)>0 ? true : false;
  }

  int Element::tree_level()
  {
    int level = 0;
    size_t pos = 0;
    while( pos != std::string::npos ){
      pos = this->get_full_name().find('/', pos+1);
      level++;
    }
    return level;
  }

  /* ================== Element Class protected methods ==================== */
  void Element::_copy(const Element& src)
  {
    this->name = src.name;
    this->attributes = src.attributes;
    this->parent = src.parent;
  }


  /* ================== Group Class public methods ==================== */
  Group::Group() : Element(), ndattr_default_container(false)
  {
  }

  Group::Group(const std::string& name) : Element(name), ndattr_default_container(false)
  {
  }

  Group::Group(const char * name) : Element(std::string(name)), ndattr_default_container(false)
  {
  }

  Group::Group(const Group& src) : ndattr_default_container(false)
  {
    this->_copy(src);
  }

  template <typename T>
  void _delete_obj(std::pair<std::string, T*> pair )
  {
    delete pair.second;
  };

  Group::~Group()
  {
    for_each(this->datasets.begin(), this->datasets.end(), _delete_obj<Dataset>);
    for_each(this->groups.begin(), this->groups.end(), _delete_obj<Group>);
  }

  Group& Group::operator=(const Group& src)
  {
    // Check for self-assignment
    if (this == &src){
      return *this;
    }
    this->_copy(src);
    return *this;
  }

  Dataset* Group::new_dset(const std::string& name)
  {
    Dataset* ds = NULL;

    // First check that a dataset or a group with this name doesn't already exist
    if ( this->name_exist(name) ) return NULL;

    // Create the object
    ds = new Dataset(name);
    ds->parent = this;

    // Insert the string, Dataset pointer pair in the datasets map.
    std::pair<std::map<std::string, Dataset*>::iterator,bool> ret;
    ret = this->datasets.insert(std::pair<std::string, Dataset*>(name, ds));

    // Check for successful insertion.
    if (ret.second == false){
      delete ds;
      return NULL;
    }
    return ds;
  }

  Dataset* Group::new_dset(const char* name)
  {
    std::string str_name(name);
    return this->new_dset(str_name);
  }

  /**
   * Create a new group, insert it into the group list, set the full path name,
   * and finally return a pointer to the newly created object.
   * Return NULL on error (group or dataset already exists or list insertion fails)
   */
  Group* Group::new_group(const std::string& name)
  {
    Group* grp = NULL;

    // First check that a dataset or a group with this name doesn't already exist
    if ( this->name_exist(name) ) return NULL;

    // Create the new group
    grp = new Group(name);
    grp->parent = this;

    // Insert the string, Dataset pointer pair in the datasets map.
    std::pair<std::map<std::string, Group*>::iterator,bool> ret;
    ret = this->groups.insert(std::pair<std::string, Group*>(name, grp));
    // Check for successful insertion.
    if (ret.second == false) return NULL;
    return grp;
  }

  Group* Group::new_group(const char * name)
  {
    std::string str_name(name);
    return this->new_group(str_name);
  }

  /**
   * Create a new HardLink, insert it into the group list, set the full path name,
   * and finally return a pointer to the newly created object.
   * Return NULL on error (hardlink already exists or list insertion fails)
   */
  HardLink* Group::new_hardlink(const std::string& name)
  {
    HardLink* hardlink = NULL;

    // First check that a dataset, group or hardlink with this name doesn't already exist
    if ( this->name_exist(name) ) return NULL;

    // Create the new hardlink
    hardlink = new HardLink(name);
    hardlink->parent = this;

    // Insert the string, Dataset pointer pair in the datasets map.
    std::pair<std::map<std::string, HardLink*>::iterator,bool> ret;
    ret = this->hardlinks.insert(std::pair<std::string, HardLink*>(name, hardlink));
    // Check for successful insertion.
    if (ret.second == false) return NULL;
    return hardlink;
  }

  HardLink* Group::new_hardlink(const char * name)
  {
    std::string str_name(name);
    return this->new_hardlink(str_name);
  }

  int Group::find_dset_ndattr(const char * ndattr_name, Dataset** dset)
  {
    std::string str_name(ndattr_name);
    return this->find_dset_ndattr(str_name, dset);
  }

  int Group::find_dset_ndattr(const std::string& ndattr_name, Dataset** dset)
  {
    // check first whether this group has a dataset with this attribute
    std::map<std::string, Dataset*>::iterator it_dsets;
    for (it_dsets = this->datasets.begin(); it_dsets != this->datasets.end(); ++it_dsets){
      // Check for this attribute name, not if it contains an attribute
      if (it_dsets->second->has_ndattr_name(ndattr_name)){
        *dset = it_dsets->second;
        return 0;
      }
    }
    // Now check if any children has a dataset with this attribute
    std::map<std::string, Group*>::iterator it_groups;
    for (it_groups = this->groups.begin(); it_groups != this->groups.end(); ++it_groups){
      if (it_groups->second->find_dset_ndattr(ndattr_name, dset) == 0){
        return 0;
      }
    }
    return -1;
  }

  int Group::find_detector_default_dset(Dataset** dset)
  {
    // check first whether this group has a default dataset
    std::map<std::string, Dataset*>::iterator it_dsets;
    for (it_dsets = this->datasets.begin(); it_dsets != this->datasets.end(); ++it_dsets){
      if (it_dsets->second->is_src_default()){
        *dset = it_dsets->second;
        return 0;
      }
    }
    // Now check if any children has a default dataset
    std::map<std::string, Group*>::iterator it_groups;
    for (it_groups = this->groups.begin(); it_groups != this->groups.end(); ++it_groups){
      if (it_groups->second->find_detector_default_dset(dset) == 0){
        return 0;
      }
    }
    return -1;
  }

  int Group::find_dset(const char* dsetname, Dataset** dest)
  {
    std::string str_dsetname = dsetname;
    return this->find_dset(str_dsetname, dest);
  }

  int Group::find_dset(std::string& dsetname, Dataset** dest)
  {
    std::map<std::string, Dataset*>::const_iterator it_dsets;
    it_dsets = this->datasets.find(dsetname);
    if (it_dsets != this->datasets.end()){
      *dest = it_dsets->second;
      return 0;
    }

    std::map<std::string, Group*>::iterator it_groups;
    int retcode = 0;
    for (it_groups = this->groups.begin(); it_groups != this->groups.end(); ++it_groups){
      retcode = it_groups->second->find_dset(dsetname, dest);
      if (retcode == 0){
       return 0;
      }
    }
    return -1;
  }

  void Group::set_default_ndattr_group()
  {
    this->ndattr_default_container = true;
  }

  Group* Group::find_ndattr_default_group()
  {
    Group * result = NULL;
    MapGroups_t::iterator it;
    for (it = this->groups.begin(); it != this->groups.end(); ++it){
      if (it->second->ndattr_default_container){
        result = it->second;
        break;
      } else {
        result = it->second->find_ndattr_default_group();
        if (result != NULL){
          break;
        }
      }
    }
    return result;
  }

  int Group::num_datasets()
  {
    return (int)this->datasets.size();
  }

  int Group::num_groups()
  {
    return (int)this->groups.size();
  }

  std::string Group::_str_()
  {
    std::stringstream out(std::stringstream::out);
    out << "< Group: \'" << this->get_full_name() << "\'";
    out << " groups=" << this->num_groups();
    out << " dsets=" << this->num_datasets();
    out << " attr=" << this->attributes.size() << ">";

    std::map<std::string, Group*>::iterator it_groups;
    for (it_groups = this->groups.begin(); it_groups != this->groups.end(); ++it_groups){
      out << "\n\t\t\t\t";
      for (int i=0; i<this->tree_level(); i++){
        out << "  ";
      }
      out << it_groups->second->_str_();
    }
    return out.str();
  }

  Group::MapDatasets_t& Group::get_datasets()
  {
    return this->datasets;
  }

  Group::MapGroups_t& Group::get_groups()
  {
    return this->groups;
  }

  Group::MapHardLinks_t& Group::get_hardlinks()
  {
    return this->hardlinks;
  }

  void Group::find_dsets(DataSrc_t source, MapDatasets_t& results)
  {
    MapDatasets_t::iterator it = this->datasets.begin();

    // Search through the dataset map to find any dataset with comes from <source>
    // Each result is inserted into the <dsets> map.
    for (it =  this->datasets.begin(); it != this->datasets.end(); ++it){
      if (it->second->data_source().is_src(source)){
        results.insert(std::pair<std::string, Dataset*>(it->second->get_full_name(), it->second));
      }
    }

    // Finally run the same search through all subgroups
    MapGroups_t::iterator it_groups;
    for (it_groups = this->groups.begin(); it_groups != this->groups.end(); ++it_groups){
      it_groups->second->find_dsets(source, results);
    }
  }

  void Group::merge_ndattributes(MapNDAttrSrc_t::const_iterator it_begin,
                                 MapNDAttrSrc_t::const_iterator it_end,
                                 std::set<std::string>& used_ndattribute_srcs)
  {
    MapNDAttrSrc_t::const_iterator it;
    MapDatasets_t::iterator it_dset;
    for (it = it_begin; it != it_end; ++it){
      it_dset = this->datasets.find(it->first);
      if (it_dset != this->datasets.end()){
        DataSource data_src(*it->second);
        it_dset->second->set_data_source(data_src);
        used_ndattribute_srcs.insert(it->first);
      }
    }

    // Recursively call the children (groups) of this group to do the same
    // operation on their datasets.
    MapGroups_t::iterator it_groups;
    for (it_groups = this->groups.begin(); it_groups != this->groups.end(); ++it_groups){
      it_groups->second->merge_ndattributes(it_begin, it_end, used_ndattribute_srcs);
    }
  }

  /* ================== Group Class private methods ==================== */
  void Group::_copy(const Group& src)
  {
    Element::_copy(src);
    this->datasets = src.datasets;
    this->groups = src.groups;
    this->ndattr_default_container = src.ndattr_default_container;
  }

  bool Group::name_exist(const std::string& name)
  {
    // First check that a dataset or a group with this name doesn't already exist
    if (this->datasets.count(name) > 0){
      return true;
    }
    if (this->groups.count(name) > 0){
      return true;
    }
    return false;
  }

  Root::Root() : Group()
  {
  }

  Root::Root(const std::string& name) : Group(name)
  {
  }

  Root::Root(const char *name) : Group(name)
  {
  }

  void Root::merge_ndattributes(MapNDAttrSrc_t::const_iterator it_begin,
                                MapNDAttrSrc_t::const_iterator it_end,
                                std::set<std::string>& used_ndattribute_srcs)
  {
    // first call the base class method
    Group::merge_ndattributes(it_begin, it_end, used_ndattribute_srcs);

    // Use the used_ndattribute_srcs set to work out which NDAttributes were not
    // already found in the defined tree. The ones that were not already defined
    // in the tree will be added as new datasets in the default NDAttribute group.

    // Find the default group for NDAttribute datasets
    Group* def_grp = this->find_ndattr_default_group();
    if (def_grp == NULL) return; // if there are no default group: then nothing left to do

    MapNDAttrSrc_t::const_iterator it;
    std::string name;
    for (it = it_begin; it != it_end; ++it){
      // Check if an attribute is not in the 'used' list - i.e. it has not been
      // used to create a dataset elsewhere already...
      name = it->first;
      if (used_ndattribute_srcs.count(name) == 0){
        // create a new dataset from the NDAttribute in the default group
        Dataset* new_dset = def_grp->new_dset(name);
        if (new_dset == NULL) continue; // one already existed so just skip to next
        new_dset->set_data_source(*it->second);
      }
    }
  }


  /* ================== Dataset Class public methods ==================== */
  Dataset::Dataset(const Dataset& src) : data_ptr(NULL), data_nelements(0),
                                         data_current_element(0), data_max_bytes(0),
                                         data_nelements_stored(0), data_default(false),
                                         ndattr_name("")
  {
    this->_copy(src);
  }

  Dataset::Dataset() : Element(), data_ptr(NULL), data_nelements(0),
                       data_current_element(0),data_max_bytes(0),
                       data_nelements_stored(0), data_default(false),
                       ndattr_name("")
  {
  }

  Dataset::Dataset(const std::string& name) : Element(name), data_ptr(NULL), data_nelements(0),
                                              data_current_element(0),data_max_bytes(0),
                                              data_nelements_stored(0), data_default(false),
                                              ndattr_name("")
  {
  }

  Dataset::~Dataset()
  {
    if (this->data_ptr != NULL) {
      free(this->data_ptr);
      this->data_ptr = NULL;
    }
  }

  Dataset& Dataset::operator =(const Dataset& src)
  {
    // Check for self-assignment
    if (this == &src){
      return *this;
    }
    this->_copy(src);
    return *this;
  }

  std::string Dataset::_str_()
  {
    unsigned int i = 0;
    DataType_t dtype;
    std::stringstream out(std::stringstream::out);
    out << "< Dataset: \'" << this->get_full_name() << "\'";
    out << " datatype: " << this->datasource.get_datatype();
    if (this->datasource.is_src_ndattribute()){
      out << " NDAttribute: \'" << this->ndattr_name << "/" << this->datasource.get_src_def() << "\'";
      out << " num/max elements: " << this->data_current_element << "/" << this->data_nelements << " ";
      out << " ptr: " << this->data_ptr << " ";
      dtype = this->data_source().get_datatype();
      if (dtype == float64){
        for (i=0; i<this->data_current_element; i++){
          out << *(((double*)this->data_ptr)+i) << ", ";
        }
      } else if (dtype == uint32 || dtype == int32){
        for (i=0; i<this->data_current_element; i++){
          out << *(((int*)this->data_ptr)+i) << ", ";
        }
      }
    } else if (this->datasource.is_src_detector()){
      out << " detector data";
    }
    out << ">";
    return out.str();
  }

  void Dataset::set_data_source(DataSource& src)
  {
    this->datasource = src;
    this->data_clear();
  }

  void Dataset::set_data_source(DataSource& src, size_t max_elements)
  {
    this->set_data_source(src);
    this->data_alloc_max_elements(max_elements);
  }

  DataSource& Dataset::data_source()
  {
    return this->datasource;
  }

  void Dataset::data_alloc_max_elements(size_t max_elements)
  {
    this->data_clear(); // make sure we're freeing memory before allocating
    this->data_max_bytes = this->datasource.datatype_size() * max_elements;
    this->data_ptr = calloc(max_elements, this->datasource.datatype_size());
    this->data_current_element = 0;
    this->data_nelements = max_elements;
  }

  size_t Dataset::data_append_value(void * val)
  {
    if (this->data_ptr == NULL) return 0;
    size_t esize = this->datasource.datatype_size();
    if (esize * this->data_current_element > this->data_max_bytes) return 0;

    // automatically re-allocate twice as much memory if we have filled up the
    // current buffer.
    if (this->data_current_element >= this->data_nelements){
      this->data_nelements *= 2;
      void * ptmp = calloc( this->data_nelements, esize);
      memcpy(ptmp, this->data_ptr, this->data_max_bytes);
      free(this->data_ptr);
      this->data_ptr = ptmp;
      this->data_max_bytes *= 2;
    }
    memcpy((char*)this->data_ptr + (esize * this->data_current_element), val, esize);
    this->data_current_element++;
    return this->data_current_element;
  }

  size_t Dataset::data_num_elements()
  {
    return this->data_nelements;
  }

  size_t Dataset::data_store_size()
  {
    return this->data_nelements + this->data_nelements_stored;
  }

  void Dataset::data_stored()
  {
    size_t tmp_nelements = this->data_nelements;
    this->data_nelements_stored += this->data_nelements;
    this->data_clear();
    this->data_alloc_max_elements(tmp_nelements);
  }

  const void * Dataset::data()
  {
    return this->data_ptr;
  }

  void Dataset::data_clear()
  {
    // Only free the memory if we have it allocated.
    if (this->data_ptr != NULL){
      free(this->data_ptr);
      this->data_ptr = NULL;
    }
    this->data_nelements = 0;
    this->data_max_bytes = 0;
    this->data_current_element = 0;
  }

  void Dataset::set_src_default(bool def)
  {
    this->data_default = def;
  }

  bool Dataset::is_src_default()
  {
    return this->data_default;
  }

  void Dataset::set_ndattr_name(const std::string& name)
  {
    ndattr_name = name;
  }

  bool Dataset::has_ndattr_name(const std::string& name)
  {
    return (ndattr_name == name);
  }

  /* ================== Dataset Class private methods ==================== */
  void Dataset::_copy(const Dataset& src)
  {
    Element::_copy(src);
    this->ndattr_name = src.ndattr_name;
    this->datasource = src.datasource;

    this->data_current_element = src.data_current_element;
    this->data_max_bytes = src.data_max_bytes;
    this->data_nelements = src.data_nelements;
    this->data_ptr = calloc( this->data_nelements, this->datasource.datatype_size() );
  }

  /* ================== HardLink Class public methods ==================== */
  HardLink::HardLink(const HardLink& src) : target("")
  {
    this->_copy(src);
  }

  HardLink::HardLink() : Element(), target("")
  {
  }

  HardLink::HardLink(const std::string& name) : Element(name), target("")
  {
  }

  HardLink::~HardLink()
  {
  }

  HardLink& HardLink::operator =(const HardLink& src)
  {
    // Check for self-assignment
    if (this == &src){
      return *this;
    }
    this->_copy(src);
    return *this;
  }

  std::string HardLink::_str_()
  {
    std::stringstream out(std::stringstream::out);
    out << "< HardLink: \'" << this->get_full_name() << "\'";
    out << " target: " << this->target;
    out << ">";
    return out.str();
  }

  void HardLink::set_target(const std::string& tgt)
  {
    target = tgt;
  }

  std::string& HardLink::get_target()
  {
    return this->target;
  }


  /* ================== HardLink Class private methods ==================== */
  void HardLink::_copy(const HardLink& src)
  {
    Element::_copy(src);
    this->target = src.target;
  }

} // hdf5


