<?xml version="1.0" encoding="UTF-8"?>

<!--
    ########### SVN repository information ###################
    # $Date$
    # $Author$
    # $Revision$
    # $HeadURL$
    # $Id$
    ########### SVN repository information ###################
-->

<!--  usage:
    xmllint - -schematron /path1/template.sch /path2/NX_template_file.xml
    (put the two minus signs together for the actual command, 
    XML can't have them together in an XML file, even in a comment!
    
    TODO:  under development
-->

<schema xmlns="http://purl.oclc.org/dsdl/schematron">
    
    <title>validate EPICS areaDetector NeXus template file</title>
    
    <!--  
        RULES
        ================
        
        + rule is implemented below
        ~ rule is approximated below
        ! rule is implied but not restricted
        
        
        !Element names can be chosen by user (any)
        ~group elements contain groups and datasets and have a name attribute or type="UserGroup" attribute
        group names are either NeXus names or non-NeXus names
        ~datasets contain optional Attr elements and optional constant text
        datasets
            +source  (required if type="ND_ATTR")
            type
            outtype
        +NeXus template files have "NXroot" as the root element
        +"NXentry" is the child element of "NXroot"
        +Non-NeXus groups (does not start with "NX") must have a type="UserGroup" attribute
        !Non-NeXus groups can be anywhere.
        +Each NeXus group must have a "name" attribute
        Each NXentry must have a unique name.
        +Each NXentry must have a "NXdata" group.
        +Each NXdata must have one and only one dataset with attribute signal="1"
        For "name" attributes:
            +must be less than 64 characters
            ~each must match the NeXus regexp:  [A-Za-z_][\w_]*
            each must be unique within the same HDF level
            some NeXus group names are not yet known to areaDetector, so use type="UserGroup" attribute for them
    -->
    
    <pattern id="name_regexp">
        <!-- Validate the name rules -->
        <rule context="*/@name">
            <assert test="string-length(.)&lt;64">name attribute must be less than 64 characters</assert>
            <report test="starts-with(.,'NX')">Do not start names with "NX" (reserved by NeXus)</report>
            <!-- TODO: test regexp   [A-Za-z_][\w_]*  --> <!-- but cannot do in Schematron -->
            <report test="starts-with(.,'0')">names must start with a letter or "_"</report>
            <report test="starts-with(.,'1')">names must start with a letter or "_"</report>
            <report test="starts-with(.,'2')">names must start with a letter or "_"</report>
            <report test="starts-with(.,'3')">names must start with a letter or "_"</report>
            <report test="starts-with(.,'4')">names must start with a letter or "_"</report>
            <report test="starts-with(.,'5')">names must start with a letter or "_"</report>
            <report test="starts-with(.,'6')">names must start with a letter or "_"</report>
            <report test="starts-with(.,'7')">names must start with a letter or "_"</report>
            <report test="starts-with(.,'8')">names must start with a letter or "_"</report>
            <report test="starts-with(.,'9')">names must start with a letter or "_"</report>
            <report test="starts-with(.,' ')">names must NOT start with whitespace</report>
            <report test="contains(normalize-space(.),' ')">names must not contain a space</report>
            <report test="contains(.,'&amp;')">names must not contain a "&amp;"</report>
            <report test="contains(.,'&gt;')">names must not contain a "&gt;"</report>
            <report test="contains(.,'&lt;')">names must not contain a "&lt;"</report>
            <report test="contains(.,'!')">names must not contain a "!"</report>
            <report test="contains(.,'#')">names must not contain a "#"</report>
            <report test="contains(.,'%')">names must not contain a "%"</report>
            <report test="contains(.,'(')">names must not contain a "("</report>
            <report test="contains(.,')')">names must not contain a ")"</report>
            <report test="contains(.,'*')">names must not contain a "*"</report>
            <report test="contains(.,'+')">names must not contain a "+"</report>
            <report test="contains(.,',')">names must not contain a ","</report>
            <report test="contains(.,'-')">names must not contain a "-"</report>
            <report test="contains(.,'.')">names must not contain a "."</report>
            <report test="contains(.,'/')">names must not contain a "/"</report>
            <report test="contains(.,':')">names must not contain a ":"</report>
            <report test="contains(.,';')">names must not contain a ";"</report>
            <report test="contains(.,'?')">names must not contain a "?"</report>
            <report test="contains(.,'@')">names must not contain a "@"</report>
            <report test="contains(.,'[')">names must not contain a "["</report>
            <report test="contains(.,'\')">names must not contain a "\"</report>
            <report test="contains(.,']')">names must not contain a "]"</report>
            <report test="contains(.,'^')">names must not contain a "^"</report>
            <report test="contains(.,'{')">names must not contain a "{"</report>
            <report test="contains(.,'|')">names must not contain a "|"</report>
            <report test="contains(.,'}')">names must not contain a "}"</report>
            <report test="contains(.,'~')">names must not contain a "~"</report>
        </rule>
    </pattern>
    
    <pattern id="NeXus_root_element">
        <!-- Ensure the document starts with /NXroot and has /NXroot/NXentry -->
        <rule context="/NXroot">
            <assert test="NXentry">/NXroot must contain at least one NXentry group</assert>
        </rule>
    </pattern> 
    
    <pattern id="NXentry_NXdata_requirements">
        <!-- Ensure the document has /NXroot/NXentry/NXdata -->
        <rule context="NXentry">
            <assert test="NXdata">NXentry must contain at least one NXdata group</assert>
        </rule>
    </pattern>
    
    <pattern id="NXdata_requirements">
        <!-- Ensure the NeXus default plottable data requirements -->
        <rule context="NXdata">
            <assert test="count(*[@type='pArray'])=1">
                NXdata must contain one dataset with type="pArray" (image data)
            </assert>
            <assert test="*/Attr[@name='signal']=1">
                NXdata must contain a dataset with attribute "signal"
            </assert>
            <!-- TODO: check that _only_ one dataset in NXdata has signal="1" attribute declared 
                Suggest to use Muenchian technique, but key attribute not supported here.
            -->
        </rule>
    </pattern>
    
    <pattern id="Attr_element">
        <!-- EPICS areaDetector declares user attributes with an Attr element -->
        <rule context="Attr">
            <report test="@type='ND_ATTR' and count(@source)=0"> 
                must provide a "source" attribute for ND_ATTR 
            </report>
            <report test="@source and @type!='ND_ATTR'">
                must use type="ND_ATTR" when specifying a source
            </report>
            
            <!-- TODO: just report the first (or last) occurence -->
            <report test="not(starts-with(name(../..),'NX')) and string(../../@type)!='UserGroup'">
                Group <value-of select="name(../..)"/> needs a type="UserGroup" attribute
            </report>
            
            <!-- TODO: just report the first (or last) occurence -->
            <report test="starts-with(name(../..),'NX') and count(../../@name)=0">
                NeXus group <value-of select="name(../..)"/> must have a "name" attribute
            </report>
        </rule>
    </pattern> 
    
</schema>
