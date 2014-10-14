#!/bin/sh

########### SVN repository information ###################
# $Date$
# $Author$
# $Revision$
# $HeadURL$
# $Id$
########### SVN repository information ###################

# Validate the XML Attribute and Template files

ATTRIBUTE_SCHEMA="--noout --schema ./NDAttributes.xsd"
NEXUS_TEMPLATE_SCHEMA="--noout --schematron ./template.sch"
HDF5_TEMPLATE_SCHEMA="--noout --schema ./hdf5_xml_layout_schema.xsd"

# NDArray Attribute declaration files
xmllint ${ATTRIBUTE_SCHEMA} ../iocs/simDetectorIOC/iocBoot/iocSimDetector/ROIAttributes.xml
xmllint ${ATTRIBUTE_SCHEMA} ../iocs/simDetectorIOC/iocBoot/iocSimDetector/netCDFAttributes.xml
xmllint ${ATTRIBUTE_SCHEMA} ../iocs/simDetectorIOC/iocBoot/iocSimDetector/simDetectorAttributes.xml

# NeXus file writer plugin template files
xmllint ${NEXUS_TEMPLATE_SCHEMA} ../iocs/simDetectorIOC/iocBoot/iocSimDetector/NexusTemplate.xml

# HDF5 file writer plugin template files
xmllint ${HDF5_TEMPLATE_SCHEMA} ../iocs/simDetectorIOC/iocBoot/iocSimDetector/hdf5_layout_demo.xml
xmllint ${HDF5_TEMPLATE_SCHEMA} ../iocs/simDetectorIOC/iocBoot/iocSimDetector/hdf5_layout_nexus.xml
