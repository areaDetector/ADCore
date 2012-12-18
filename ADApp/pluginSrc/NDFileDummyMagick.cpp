/* This is a dummy routine for systems like vxWorks that don't support JPEG, Nexus or TIFF */

#include <epicsExport.h>

extern "C" void NDFileMagickRegister(void)
{
}
extern "C" {
epicsExportRegistrar(NDFileMagickRegister);
}
