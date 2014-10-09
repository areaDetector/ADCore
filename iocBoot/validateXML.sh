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
#TEMPLATE_SCHEMA="--noout --schema ./NeXus_templates.xsd"
TEMPLATE_SCHEMA="--noout --schematron ./template.sch"

# NDArray Attribute declaration files
mllint ${ATTRIBUTE_SCHEMA} ../iocs/simDetectorIOC/iocBoot/iocSimDetector/ROIAttributes.xml
xmllint ${ATTRIBUTE_SCHEMA} ../iocs/simDetectorIOC/iocBoot/iocSimDetector/netCDFAttributes.xml
xmllint ${ATTRIBUTE_SCHEMA} ../iocs/simDetectorIOC/iocBoot/iocSimDetector/simDetectorAttributes.xml

# NeXus file writer plugin template files
#
xmllint ${TEMPLATE_SCHEMA} ../iocs/simDetectorIOC/iocBoot/iocSimDetector/NexusTemplate.xml
