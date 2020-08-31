/* This is a generated file, do not edit! */

#ifndef INC_ADCoreAPI_H
#define INC_ADCoreAPI_H

#if defined(_WIN32) || defined(__CYGWIN__)

#  if !defined(epicsStdCall)
#    define epicsStdCall __stdcall
#  endif

#  if defined(BUILDING_ADCore_API) && defined(EPICS_BUILD_DLL)
/* Building library as dll */
#    define ADCORE_API __declspec(dllexport)
#  elif !defined(BUILDING_ADCore_API) && defined(EPICS_CALL_DLL)
/* Calling library in dll form */
#    define ADCORE_API __declspec(dllimport)
#  endif

#elif __GNUC__ >= 4
#  define ADCORE_API __attribute__ ((visibility("default")))
#endif

#if !defined(ADCORE_API)
#  define ADCORE_API
#endif

#if !defined(epicsStdCall)
#  define epicsStdCall
#endif

#endif /* INC_ADCoreAPI_H */

