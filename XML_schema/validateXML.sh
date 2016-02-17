#!/bin/sh

# Validate the XML Attribute and Nexus and HDF5 Template files

ATTRIBUTE_SCHEMA="--noout --schema ./NDAttributes.xsd"
NEXUS_TEMPLATE_SCHEMA="--noout --schematron ./template.sch"
HDF5_TEMPLATE_SCHEMA="--noout --schema ./hdf5_xml_layout_schema.xsd"

# NDArray Attribute declaration files
xmllint ${ATTRIBUTE_SCHEMA} ../../ADExample/iocs/simDetectorIOC/iocBoot/iocSimDetector/ROIAttributes.xml
xmllint ${ATTRIBUTE_SCHEMA} ../../ADExample/iocs/simDetectorIOC/iocBoot/iocSimDetector/netCDFAttributes.xml
xmllint ${ATTRIBUTE_SCHEMA} ../../ADExample/iocs/simDetectorIOC/iocBoot/iocSimDetector/simDetectorAttributes.xml

# NeXus file writer plugin template files
xmllint ${NEXUS_TEMPLATE_SCHEMA} ../../ADExample/iocs/simDetectorIOC/iocBoot/iocSimDetector/NexusTemplate.xml

# HDF5 file writer plugin template files
xmllint ${HDF5_TEMPLATE_SCHEMA} ../../ADExample/iocs/simDetectorIOC/iocBoot/iocSimDetector/hdf5_layout_demo.xml
xmllint ${HDF5_TEMPLATE_SCHEMA} ../../ADExample/iocs/simDetectorIOC/iocBoot/iocSimDetector/hdf5_layout_nexus.xml
