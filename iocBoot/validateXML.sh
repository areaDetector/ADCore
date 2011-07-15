#!/bin/sh

########### SVN repository information ###################
# $Date$
# $Author$
# $Revision$
# $HeadURL$
# $Id$
########### SVN repository information ###################

# Validate the XML Attribute and Template files


# NDArray Attribute declaration files
xmllint --noout --schema ./attributes.xsd iocPerkinElmer/nexus_templates/PerkinElmerParams.xml
xmllint --noout --schema ./attributes.xsd iocPilatus/pilatusAttributes.xml
xmllint --noout --schema ./attributes.xsd iocProsilica/prosilicaAttributes.xml
xmllint --noout --schema ./attributes.xsd iocSimDetector/ROIAttributes.xml
xmllint --noout --schema ./attributes.xsd iocSimDetector/netCDFAttributes.xml
xmllint --noout --schema ./attributes.xsd iocSimDetector/simDetectorAttributes.xml

# template files
#
# cannot validate, no XML Schema developed yet
#
#iocPerkinElmer/nexus_templates/example.xml fails to validate
#iocSimDetector/NexusTemplate.xml fails to validate
