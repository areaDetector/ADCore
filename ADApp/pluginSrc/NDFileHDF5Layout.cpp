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

namespace hdf5 {

/* ================== HdfAttribute Class public methods ==================== */
HdfAttribute::HdfAttribute(const HdfAttribute& src)
{
	this->_copy(src);
}
HdfAttribute::HdfAttribute(std::string& name)
: name(name), onFileOpen(true) {}
HdfAttribute::HdfAttribute(const char* name)
: name(name), onFileOpen(true) {}

/** Assignment operator */
HdfAttribute& HdfAttribute::operator=(const HdfAttribute& src)
{
	this->_copy(src);
	return *this;
}
std::string HdfAttribute::get_name() {return this->name;}

void HdfAttribute::setOnFileOpen(bool onOpen)
{
  this->onFileOpen = onOpen;
}

bool HdfAttribute::is_onFileOpen()
{
  return this->onFileOpen;
}

bool HdfAttribute::is_onFileClose()
{
  return !this->onFileOpen;
}

// constructors
HdfDataSource::HdfDataSource()
: data_src(hdf_notset), val(""), datatype(hdf_int8){}
HdfDataSource::HdfDataSource( HdfDataSrc_t srctype, const std::string& val)
: data_src(srctype), val(val), datatype(hdf_string){}
HdfDataSource::HdfDataSource( HdfDataSrc_t src, HDF_DataType_t type)
: data_src(src), val(""), datatype(type){}
HdfDataSource::HdfDataSource( HdfDataSrc_t src)
: data_src(src), val(""), datatype(hdf_string){}
HdfDataSource::HdfDataSource( const HdfDataSource& src)
: data_src(src.data_src), val(src.val), datatype(src.datatype){}

/** Assignment operator
 * Copies the sources private data members to this object.
 */
HdfDataSource& HdfDataSource::operator=(const HdfDataSource& src)
{
	this->data_src = src.data_src;
	this->val = src.val;
	this->datatype = src.datatype;
	return *this;
};

void HdfDataSource::set_datatype(HDF_DataType_t type)
{
	this->datatype = type;
}

bool HdfDataSource::is_src_constant() {
    return this->data_src == hdf_constant ? true : false;
}
bool HdfDataSource::is_src_detector() {
    return this->data_src == hdf_detector ? true : false;
}
bool HdfDataSource::is_src_ndattribute() {
    return this->data_src == hdf_ndattribute ? true : false;
}
bool HdfDataSource::is_src(HdfDataSrc_t src)
{
	return this->data_src == src ? true : false;
}

std::string HdfDataSource::get_src_def()
{
    return this->val;
}

HDF_DataType_t HdfDataSource::get_datatype()
{
	return this->datatype;
}

size_t HdfDataSource::datatype_size()
{
	size_t retval = sizeof(char);
	switch (this->datatype)
	{
	case hdf_uint8:
	case hdf_int8:
		retval = 1; break;
	case hdf_uint16:
	case hdf_int16:
		retval = 2; break;
	case hdf_uint32:
	case hdf_int32:
	case hdf_float32:
		retval = 4; break;
	case hdf_float64:
		retval = 8; break;
	case hdf_string:
		retval = 1; break;
	default:
		retval = 0; break;
	}
	return retval;
}

void HdfDataSource::set_const_datatype_value(HDF_DataType_t dtype, const std::string& str_val)
{
	if (this->data_src != hdf_constant) return;
	this->set_datatype(dtype);
	this->val = str_val;
}


/* ================== HdfElement Class public methods ==================== */
HdfElement::HdfElement()
{
    this->name = "";
    this->parent = NULL;
}

HdfElement::HdfElement(const HdfElement& src)
{
	this->_copy(src);
}

HdfElement::HdfElement(const std::string& name)
{
    this->name = name;
    this->parent = NULL;
}

std::string HdfElement::get_full_name()
{
	std::string fname = this->get_path(true);
	fname += this->name;
    return fname;
}

std::string HdfElement::get_path(bool trailing_slash)
{
	std::string path;
	path.append("/");
	if (this->parent != NULL) {
		path.insert(0, this->parent->get_name());
		path.insert(0, this->parent->get_path(true));
		if (not trailing_slash) path.erase(path.end() - 1);
	}
	return path;
}

HdfElement& HdfElement::operator=(const HdfElement& src)
{
    // Check for self-assignment
    if (this == &src)
        return *this;
    this->_copy(src);
    return *this;
}

const std::string& HdfElement::get_name()
{
	return this->name;
}

HdfElement * HdfElement::get_parent()
{
	return this->parent;
}

HdfElement::MapAttributes_t& HdfElement::get_attributes()
{
	return this->attributes;
}

int HdfElement::add_attribute(HdfAttribute& attr)
{
    if (this->attributes.count(attr.get_name()) != 0) return -1;
    std::pair<std::map<std::string,HdfAttribute>::iterator,bool> ret;
    ret = this->attributes.insert( std::pair<std::string, HdfAttribute>(attr.get_name(), attr) );
    // Check for successful insertion.
    if (ret.second == false) return -1;
    return 0;
}

bool HdfElement::has_attribute(const std::string& attr_name)
{
    return this->attributes.count(attr_name)>0 ? true : false;
}

int HdfElement::tree_level()
{
    int level = 0;
    size_t pos = 0;
    while( pos != std::string::npos ){
        pos = this->get_full_name().find('/', pos+1);
        level++;
    }
    return level;
}

/* ================== HdfElement Class protected methods ==================== */
void HdfElement::_copy(const HdfElement& src)
{
    this->name = src.name;
    this->attributes = src.attributes;
    this->parent = src.parent;
}


/* ================== HdfGroup Class public methods ==================== */
HdfGroup::HdfGroup()
: HdfElement(), ndattr_default_container(false){}
HdfGroup::HdfGroup(const std::string& name)
: HdfElement(name), ndattr_default_container(false){}
HdfGroup::HdfGroup(const char * name)
: HdfElement( std::string(name)), ndattr_default_container(false) {}

HdfGroup::HdfGroup(const HdfGroup& src)
: ndattr_default_container(false)
{
    this->_copy(src);
}

template <typename T>
void _delete_obj(std::pair<std::string, T*> pair )
{
    delete pair.second;
};

HdfGroup::~HdfGroup()
{
    for_each(this->datasets.begin(), this->datasets.end(), _delete_obj<HdfDataset>);
    for_each(this->groups.begin(), this->groups.end(), _delete_obj<HdfGroup>);

}

HdfGroup& HdfGroup::operator=(const HdfGroup& src)
{
    // Check for self-assignment
    if (this == &src)
        return *this;
    this->_copy(src);
    return *this;
}


HdfDataset* HdfGroup::new_dset(const std::string& name)
{
    HdfDataset* ds = NULL;

    // First check that a dataset or a group with this name doesn't already exist
    if ( this->name_exist(name) ) return NULL;

    // Create the object
    ds = new HdfDataset(name);
    ds->parent = this;

    // Insert the string, HdfDataset pointer pair in the datasets map.
    std::pair<std::map<std::string,HdfDataset*>::iterator,bool> ret;
    ret = this->datasets.insert( std::pair<std::string, HdfDataset*>(name, ds) );

    // Check for successful insertion.
    if (ret.second == false) return NULL;
    return ds;
}

HdfDataset* HdfGroup::new_dset(const char* name)
{
    std::string str_name(name);
    return this->new_dset(str_name);
}


/** Create a new group, insert it into the group list, set the full path name,
 * and finally return a pointer to the newly created object.
 * Return NULL on error (group or dataset already exists or list insertion fails)
 */
HdfGroup* HdfGroup::new_group(const std::string& name)
{
    HdfGroup* grp = NULL;

    // First check that a dataset or a group with this name doesn't already exist
    if ( this->name_exist(name) ) return NULL;

    // Create the new group
    grp = new HdfGroup(name);
    grp->parent = this;

    // Insert the string, HdfDataset pointer pair in the datasets map.
    std::pair<std::map<std::string,HdfGroup*>::iterator,bool> ret;
    ret = this->groups.insert( std::pair<std::string, HdfGroup*>(name, grp) );
    // Check for successful insertion.
    if (ret.second == false) return NULL;
    return grp;
}

HdfGroup* HdfGroup::new_group(const char * name)
{
    std::string str_name(name);
    return this->new_group(str_name);
}

int HdfGroup::find_dset_ndattr(const char * ndattr_name, HdfDataset** dset)
{
	std::string str_name(ndattr_name);
	return this->find_dset_ndattr(str_name, dset);
}


int HdfGroup::find_dset_ndattr(const std::string& ndattr_name, HdfDataset** dset)
{
    // check first whether this group has a dataset with this attribute
    std::map<std::string, HdfDataset*>::iterator it_dsets;
    for (it_dsets = this->datasets.begin();
         it_dsets != this->datasets.end();
         ++it_dsets)
    {
//        if ( it_dsets->second->has_attribute( ndattr_name ) )
//        {
//            *dset = it_dsets->second;
//            return 0;
//        }

// Check for this attribute name, not if it contains an attribute
        if ( it_dsets->second->has_ndattr_name( ndattr_name ) )
        {
            *dset = it_dsets->second;
            return 0;
        }
    }

    // Now check if any children has a dataset with this attribute
    std::map<std::string, HdfGroup*>::iterator it_groups;
    for (it_groups = this->groups.begin();
         it_groups != this->groups.end();
         ++it_groups)
    {
        if (it_groups->second->find_dset_ndattr(ndattr_name, dset) == 0)
        {
            return 0;
        }
    }
    return -1;
}

int HdfGroup::find_detector_default_dset(HdfDataset** dset)
{
    // check first whether this group has a default dataset
    std::map<std::string, HdfDataset*>::iterator it_dsets;
    for (it_dsets = this->datasets.begin();
         it_dsets != this->datasets.end();
         ++it_dsets)
    {
        if ( it_dsets->second->is_src_default() )
        {
            *dset = it_dsets->second;
            return 0;
        }
    }

    // Now check if any children has a default dataset
    std::map<std::string, HdfGroup*>::iterator it_groups;
    for (it_groups = this->groups.begin();
         it_groups != this->groups.end();
         ++it_groups)
    {
        if (it_groups->second->find_detector_default_dset(dset) == 0)
        {
            return 0;
        }
    }
    return -1;
}

int HdfGroup::find_dset( const char* dsetname, HdfDataset** dest)
{
	std::string str_dsetname = dsetname;
	return this->find_dset(str_dsetname, dest);
}

int HdfGroup::find_dset( std::string& dsetname, HdfDataset** dest )
{
    std::map<std::string, HdfDataset*>::const_iterator it_dsets;
    it_dsets = this->datasets.find(dsetname);
    if (it_dsets != this->datasets.end())
    {
        *dest = it_dsets->second;
        return 0;
    }

    std::map<std::string, HdfGroup*>::iterator it_groups;
    int retcode = 0;
    for (it_groups = this->groups.begin();
            it_groups != this->groups.end();
            ++it_groups)
    {
        retcode = it_groups->second->find_dset(dsetname, dest);
        if (retcode == 0) return 0;
    }
    return -1;
}

void HdfGroup::set_default_ndattr_group()
{
	this->ndattr_default_container = true;
}
HdfGroup* HdfGroup::find_ndattr_default_group()
{
	HdfGroup * result = NULL;
	MapGroups_t::iterator it;
	for (it = this->groups.begin(); it != this->groups.end(); ++it)
	{
		if (it->second->ndattr_default_container) {
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

int HdfGroup::num_datasets()
{
    return this->datasets.size();
}

int HdfGroup::num_groups()
{
    return this->groups.size();
}

std::string HdfGroup::_str_()
{
    std::stringstream out(std::stringstream::out);
    out << "< HdfGroup: \'" << this->get_full_name() << "\'";
    out << " groups=" << this->num_groups();
    out << " dsets=" << this->num_datasets();
    out << " attr=" << this->attributes.size() << ">";
    //out << " level=" << this->tree_level();

    std::map<std::string, HdfGroup*>::iterator it_groups;
    for (it_groups = this->groups.begin();
            it_groups != this->groups.end();
            ++it_groups)
    {
        out << "\n\t\t\t\t";
        for (int i=0; i<this->tree_level(); i++) out << "  ";
        out << it_groups->second->_str_();
    }

    return out.str();
}

HdfGroup::MapDatasets_t& HdfGroup::get_datasets()
{
	return this->datasets;
}
HdfGroup::MapGroups_t& HdfGroup::get_groups()
{
	return this->groups;
}

void HdfGroup::find_dsets(HdfDataSrc_t source, MapDatasets_t& results)
{
	MapDatasets_t::iterator it = this->datasets.begin();

	// Search through the dataset map to find any dataset with comes from <source>
	// Each result is inserted into the <dsets> map.
	for (	it =  this->datasets.begin();
			it != this->datasets.end();
			++it)
	{
		if (it->second->data_source().is_src(source))
		{
			results.insert(std::pair<std::string, HdfDataset*>(it->second->get_full_name(), it->second));
		}
	}

	// Finally run the same search through all subgroups
	MapGroups_t::iterator it_groups;
	for ( it_groups = this->groups.begin();
			it_groups != this->groups.end();
			++it_groups)
	{
		it_groups->second->find_dsets(source, results);
	}
}

void HdfGroup::merge_ndattributes(MapNDAttrSrc_t::const_iterator it_begin,
								  MapNDAttrSrc_t::const_iterator it_end,
    							  std::set<std::string>& used_ndattribute_srcs)
{
	MapNDAttrSrc_t::const_iterator it;
	MapDatasets_t::iterator it_dset;
	for (it = it_begin; it != it_end; ++it)
	{

		it_dset = this->datasets.find(it->first);
		if (it_dset != this->datasets.end())
		{
			HdfDataSource data_src(*it->second);
			it_dset->second->set_data_source(data_src);
			used_ndattribute_srcs.insert(it->first);
		}
	}

	// Recursively call the children (groups) of this group to do the same
	// operation on their datasets.
	MapGroups_t::iterator it_groups;
	for (it_groups = this->groups.begin(); it_groups != this->groups.end(); ++it_groups)
	{
		it_groups->second->merge_ndattributes(it_begin, it_end, used_ndattribute_srcs);
	}

}



/* ================== HdfGroup Class private methods ==================== */
void HdfGroup::_copy(const HdfGroup& src)
{
    HdfElement::_copy(src);
    this->datasets = src.datasets;
    this->groups = src.groups;
    this->ndattr_default_container = src.ndattr_default_container;
}



bool HdfGroup::name_exist(const std::string& name)
{
    // First check that a dataset or a group with this name doesn't already exist
    if ( this->datasets.count(name) > 0 )
        return true;
    if ( this->groups.count(name) > 0 )
        return true;
    return false;
}

HdfRoot::HdfRoot()
: HdfGroup::HdfGroup(){}
HdfRoot::HdfRoot(const std::string& name)
: HdfGroup::HdfGroup(name){}
HdfRoot::HdfRoot(const char *name)
: HdfGroup::HdfGroup(name){}


void HdfRoot::merge_ndattributes(MapNDAttrSrc_t::const_iterator it_begin,
    								MapNDAttrSrc_t::const_iterator it_end,
    								std::set<std::string>& used_ndattribute_srcs)
{
	// first call the base class method
	HdfGroup::merge_ndattributes(it_begin, it_end, used_ndattribute_srcs);

	// Use the used_ndattribute_srcs set to work out which NDAttributes were not
	// already found in the defined tree. The ones that were not already defined
	// in the tree will be added as new datasets in the default NDAttribute group.

	// Find the default group for NDAttribute datasets
	HdfGroup* def_grp = this->find_ndattr_default_group();
	if (def_grp == NULL) return; // if there are no default group: then nothing left to do

	MapNDAttrSrc_t::const_iterator it;
	std::string name;
	for (it = it_begin; it != it_end; ++it)
	{
		// Check if an attribute is not in the 'used' list - i.e. it has not been
		// used to create a dataset elsewhere already...
		name = it->first;
		if (used_ndattribute_srcs.count(name) == 0)
		{
			// create a new dataset from the NDAttribute in the default group
			HdfDataset* new_dset = def_grp->new_dset(name);
			if (new_dset == NULL) continue; // one already existed so just skip to next
			new_dset->set_data_source(*it->second);
		}
	}


}


/* ================== HdfDataset Class public methods ==================== */
HdfDataset::HdfDataset(const HdfDataset& src)
: data_ptr(NULL), data_nelements(0),
  data_current_element(0), data_max_bytes(0),
  data_nelements_stored(0),
  data_default(false), ndattr_name("")
{
    this->_copy(src);
}

HdfDataset::HdfDataset()
: HdfElement(),
  data_ptr(NULL), data_nelements(0),
  data_current_element(0),data_max_bytes(0),
  data_nelements_stored(0),
  data_default(false), ndattr_name("")
{}

HdfDataset::HdfDataset(const std::string& name)
: HdfElement(name),
  data_ptr(NULL), data_nelements(0),
  data_current_element(0),data_max_bytes(0),
  data_nelements_stored(0),
  data_default(false), ndattr_name("")
{}

HdfDataset::~HdfDataset()
{
	if (this->data_ptr != NULL) {
		free(this->data_ptr);
		this->data_ptr = NULL;
	}
}

HdfDataset& HdfDataset::operator =(const HdfDataset& src)
{
    // Check for self-assignment
    if (this == &src)
        return *this;
    this->_copy(src);
    return *this;
}

std::string HdfDataset::_str_()
{
	unsigned int i = 0;
	HDF_DataType_t dtype;
    std::stringstream out(std::stringstream::out);
    out << "< HdfDataset: \'" << this->get_full_name() << "\'";
    out << " datatype: " << this->datasource.get_datatype();
    if (this->datasource.is_src_ndattribute())
    {
    	out << " NDAttribute: \'" << this->ndattr_name << "/" << this->datasource.get_src_def() << "\'";
    	out << " num/max elements: " << this->data_current_element << "/" << this->data_nelements << " ";
    	out << " ptr: " << this->data_ptr << " ";
    	dtype = this->data_source().get_datatype();
    	if (dtype == hdf_float64)
    		for (i=0; i<this->data_current_element; i++)
    			out << *(((double*)this->data_ptr)+i) << ", ";
    	else if (dtype == hdf_uint32 || dtype == hdf_int32)
    		for (i=0; i<this->data_current_element; i++)
    			out << *(((int*)this->data_ptr)+i) << ", ";
    } else if (this->datasource.is_src_detector())
    {
    	out << " detector data";
    }
    out << ">";
    return out.str();
}

void HdfDataset::set_data_source(HdfDataSource& src)
{
    this->datasource = src;
    this->data_clear();
}

void HdfDataset::set_data_source(HdfDataSource& src, size_t max_elements)
{
	this->set_data_source(src);
	this->data_alloc_max_elements(max_elements);
}

HdfDataSource& HdfDataset::data_source()
{
	return this->datasource;
}

void HdfDataset::data_alloc_max_elements(size_t max_elements)
{
	this->data_clear(); // make sure we're freeing memory before allocating
	this->data_max_bytes = this->datasource.datatype_size() * max_elements;
	this->data_ptr = calloc(max_elements, this->datasource.datatype_size());
	this->data_current_element = 0;
	this->data_nelements = max_elements;
}

size_t HdfDataset::data_append_value(void * val)
{
	if (this->data_ptr == NULL) return 0;
	size_t esize = this->datasource.datatype_size();
	if (esize * this->data_current_element > this->data_max_bytes) return 0;

	// automatically re-allocate twice as much memory if we have filled up the
	// current buffer.
	if (this->data_current_element >= this->data_nelements)
	{
		this->data_nelements *= 2;
		void * ptmp = calloc( this->data_nelements, esize);
		memcpy( ptmp, this->data_ptr, this->data_max_bytes );
		free(this->data_ptr);
		this->data_ptr = ptmp;
		this->data_max_bytes *= 2;
	}

	memcpy((char*)this->data_ptr + (esize * this->data_current_element), val, esize);
	this->data_current_element++;
	return this->data_current_element;
}

size_t HdfDataset::data_num_elements()
{
	return this->data_nelements;
}

size_t HdfDataset::data_store_size()
{
	return this->data_nelements + this->data_nelements_stored;
}

void HdfDataset::data_stored()
{
	size_t tmp_nelements = this->data_nelements;
	this->data_nelements_stored += this->data_nelements;
	this->data_clear();
	this->data_alloc_max_elements(tmp_nelements);
}


const void * HdfDataset::data()
{
	return this->data_ptr;
}

void HdfDataset::data_clear()
{
	// Only free the memory if we have it allocated.
	if (this->data_ptr != NULL) {
		free(this->data_ptr);
		this->data_ptr = NULL;
	}
	this->data_nelements = 0;
	this->data_max_bytes = 0;
	this->data_current_element = 0;
}

void HdfDataset::set_src_default(bool def)
{
    this->data_default = def;
}

bool HdfDataset::is_src_default()
{
    return this->data_default;
}

void HdfDataset::set_ndattr_name(const std::string& name)
{
  ndattr_name = name;
}

bool HdfDataset::has_ndattr_name(const std::string& name)
{
  return (ndattr_name == name);
}

/* ================== HdfDataset Class private methods ==================== */
void HdfDataset::_copy(const HdfDataset& src)
{
    HdfElement::_copy(src);
    this->ndattr_name = src.ndattr_name;
    this->datasource = src.datasource;

    this->data_current_element = src.data_current_element;
    this->data_max_bytes = src.data_max_bytes;
    this->data_nelements = src.data_nelements;
    this->data_ptr = calloc( this->data_nelements, this->datasource.datatype_size() );

}

} // hdf5


