#!/bin/sh

########### SVN repository information ###################
# $Date$
# $Author$
# $Revision$
# $HeadURL$
# $Id$
########### SVN repository information ###################

# Validate the XML Attribute and Template files

ATTRIBUTE_SCHEMA="--noout --schema ./attributes.xsd"
#TEMPLATE_SCHEMA="--noout --schema ./NeXus_templates.xsd"
TEMPLATE_SCHEMA="--noout --schematron ./template.sch"

# NDArray Attribute declaration files
xmllint ${ATTRIBUTE_SCHEMA} iocPerkinElmer/nexus_templates/PerkinElmerParams.xml
xmllint ${ATTRIBUTE_SCHEMA} iocPilatus/pilatusAttributes.xml
xmllint ${ATTRIBUTE_SCHEMA} iocProsilica/prosilicaAttributes.xml
xmllint ${ATTRIBUTE_SCHEMA} iocSimDetector/ROIAttributes.xml
xmllint ${ATTRIBUTE_SCHEMA} iocSimDetector/netCDFAttributes.xml
xmllint ${ATTRIBUTE_SCHEMA} iocSimDetector/simDetectorAttributes.xml

# NeXus file writer plugin template files
#
xmllint ${TEMPLATE_SCHEMA} iocPerkinElmer/nexus_templates/example.xml
xmllint ${TEMPLATE_SCHEMA} iocSimDetector/NexusTemplate.xml
