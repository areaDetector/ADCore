/*
 * layout.h
 *
 *  Created on: 23 Jan 2012
 *      Author: up45
 */

#ifndef LAYOUT_H_
#define LAYOUT_H_

#include <string>
#include <vector>
#include <map>
#include <set>

namespace hdf5 {

typedef enum {
   OnFrame,
   OnFileOpen,
   OnFileClose
} HdfWhen_t;

typedef enum {
	hdf_notset,
	hdf_detector,
	hdf_ndattribute,
	hdf_constant
}HdfDataSrc_t;

typedef enum {
	hdf_int8,
	hdf_uint8,
	hdf_int16,
	hdf_uint16,
	hdf_int32,
	hdf_uint32,
	hdf_float32,
	hdf_float64,
	hdf_string
} HDF_DataType_t;

class HdfDataSource {
public:
	// Default constructor
    HdfDataSource();
    // Initialising constructor
    HdfDataSource( HdfDataSrc_t src, const std::string& val);
    HdfDataSource( HdfDataSrc_t src);
    HdfDataSource( HdfDataSrc_t src, HDF_DataType_t type);
    // Copy constructor
    HdfDataSource( const HdfDataSource& src);
    // Assignment operator
    HdfDataSource& operator=(const HdfDataSource& src);
    ~HdfDataSource(){};
    void set_datatype(HDF_DataType_t type);
    bool is_src_detector();
    bool is_src_ndattribute();
    bool is_src_constant();
    bool is_src(HdfDataSrc_t src);

    std::string get_src_def(); /** return the string that define the source: either name of NDAttribute or constant value */
    HDF_DataType_t get_datatype();
    size_t datatype_size();
    void set_const_datatype_value(HDF_DataType_t dtype, const std::string& str_val);

    void set_when_to_save(HdfWhen_t when);
    HdfWhen_t get_when_to_save();
   
private:
    HdfDataSrc_t data_src;
    std::string val;
    HDF_DataType_t datatype;
    HdfWhen_t when_to_save;
};


class HdfAttribute {
public:
    HdfAttribute(){};
    HdfAttribute(const HdfAttribute& src); // Copy constructor
    HdfAttribute(std::string& name);
    HdfAttribute(const char* name);
    ~HdfAttribute(){};
    HdfAttribute& operator=(const HdfAttribute& src);
    std::string get_name();
    void setOnFileOpen(bool onOpen);
    bool is_onFileOpen();
    bool is_onFileClose();


    HdfDataSource source;
private:
    void _copy(const HdfAttribute& src){this->name = src.name; this->source = src.source; this->onFileOpen = src.onFileOpen;};
    std::string name;
    bool onFileOpen;
};

/** Describe a generic HDF5 structure element.
 * An element can contain a number of attributes and
 * be a subset of a number of parents.
 */
class HdfElement {
public:
    HdfElement();
    HdfElement(const HdfElement& src);
    HdfElement(const std::string& name);
    ~HdfElement(){};
    HdfElement& operator=(const HdfElement& src);

    const std::string& get_name();
    virtual std::string get_full_name();
    virtual std::string get_path(bool trailing_slash=false);
    int add_attribute(HdfAttribute& attr);
    bool has_attribute(const std::string& attr_name);
    int tree_level();
    HdfElement *get_parent();
    typedef std::map<std::string, HdfAttribute> MapAttributes_t;
    MapAttributes_t& get_attributes();

protected:
    void _copy(const HdfElement& src);
    MapAttributes_t attributes;
    std::string name;
public:
    friend class HdfGroup;
private:
    HdfElement *parent;
};


class HdfDataset: public HdfElement {
public:
    HdfDataset();
    HdfDataset(const std::string& name);
    HdfDataset(const HdfDataset& src);
    HdfDataset& operator=(const HdfDataset& src);
    ~HdfDataset();

    //void is_detector_data();

    /** Stream operator: use to prints a string representation of this class */
    inline friend std::ostream& operator<<(std::ostream& out, HdfDataset& dset)
    { out << dset._str_(); return out; }
    std::string _str_();  /** Return a string representation of the object */

    void set_data_source(HdfDataSource& src);
    void set_data_source(HdfDataSource& src, size_t max_elements);
    HdfDataSource& data_source();

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
    void _copy(const HdfDataset& src);
    HdfDataSource datasource;

    void data_clear();
    void * data_ptr;
    size_t data_nelements;
    size_t data_current_element;
    size_t data_max_bytes;
    size_t data_nelements_stored;
    bool data_default;
    std::string ndattr_name;
};

/** Describe a HDF5 group element.
 * A group is like a directory in a file system. It can contain
 * other groups and datasets (like files).
 */
class HdfGroup: public HdfElement {
public:
    HdfGroup();
    HdfGroup(const std::string& name);
    HdfGroup(const char * name);
    HdfGroup(const HdfGroup& src);
    virtual ~HdfGroup();
    HdfGroup& operator=(const HdfGroup& src);

    HdfDataset* new_dset(const std::string& name);
    HdfDataset* new_dset(const char * name);
    HdfGroup* new_group(const std::string& name);
    HdfGroup* new_group(const char * name);
    int find_dset_ndattr(const std::string& ndattr_name, HdfDataset** dset); /** << Find and return a reference to the dataset for a given NDAttribute */
    int find_dset_ndattr(const char * ndattr_name, HdfDataset** dset);
    int find_dset( std::string& dsetname, HdfDataset** dest);
    int find_dset( const char* dsetname, HdfDataset** dest);
    void set_default_ndattr_group();
    HdfGroup* find_ndattr_default_group(); //** << search through subgroups to return a pointer to the NDAttribute default container group
    int find_detector_default_dset(HdfDataset** dset);
    int num_groups();
    int num_datasets();

    /** Stream operator: use to prints a string representation of this class */
    inline friend std::ostream& operator<<(std::ostream& out, HdfGroup& grp)
    { out << grp._str_(); return out; }
    std::string _str_();  /** Return a string representation of the object */

    typedef std::map<std::string, HdfGroup*> MapGroups_t;
    typedef std::map<std::string, HdfDataset*> MapDatasets_t;
    MapGroups_t& get_groups();
    MapDatasets_t& get_datasets();
    void find_dsets(HdfDataSrc_t source, MapDatasets_t& dsets); //** return a map of datasets <string name, HDfDataset dset> which contains all datasets, marked as <source> data.

    typedef std::map<std::string, HdfDataSource*> MapNDAttrSrc_t;
    virtual void merge_ndattributes(MapNDAttrSrc_t::const_iterator it_begin,
    								MapNDAttrSrc_t::const_iterator it_end,
    								std::set<std::string>& used_ndattribute_srcs);

private:
    void _copy(const HdfGroup& src);
    bool name_exist(const std::string& name);
    bool ndattr_default_container;
    std::map<std::string, HdfDataset*> datasets;
    std::map<std::string, HdfGroup*> groups;
};

class HdfRoot: public HdfGroup {
public:
	HdfRoot();
	HdfRoot(const std::string& name);
	HdfRoot(const char *name);
	virtual ~HdfRoot(){};
    virtual void merge_ndattributes(MapNDAttrSrc_t::const_iterator it_begin,
    								MapNDAttrSrc_t::const_iterator it_end,
    								std::set<std::string>& used_ndattribute_srcs);
  std::string get_full_name(){ return ""; };
  std::string get_path(bool trailing_slash=false){ return ""; };
};

} // hdf5


#endif /* LAYOUT_H_ */
