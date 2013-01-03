/* This is a dummy routine for systems like vxWorks that don't support JPEG, Nexus or TIFF */

#include <epicsExport.h>

extern "C" void NDFileJPEGRegister(void)
{
}
extern "C" {
epicsExportRegistrar(NDFileJPEGRegister);
}

extern "C" void NDFileNexusRegister(void)
{
}
extern "C" {
epicsExportRegistrar(NDFileNexusRegister);
}

extern "C" void NDFileTIFFRegister(void)
{
}
extern "C" {
epicsExportRegistrar(NDFileTIFFRegister);
}

extern "C" void NDFileHDF5Register(void)
{
}
extern "C" {
epicsExportRegistrar(NDFileHDF5Register);
}

extern "C" void NDFileNullRegister(void)
{
}
extern "C" {
epicsExportRegistrar(NDFileNullRegister);
}
