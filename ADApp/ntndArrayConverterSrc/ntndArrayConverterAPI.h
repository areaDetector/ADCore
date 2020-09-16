/* This is a generated file, do not edit! */

#ifndef INC_ntndArrayConverterAPI_H
#define INC_ntndArrayConverterAPI_H

#if defined(_WIN32) || defined(__CYGWIN__)

#  if !defined(epicsStdCall)
#    define epicsStdCall __stdcall
#  endif

#  if defined(BUILDING_ntndArrayConverter_API) && defined(EPICS_BUILD_DLL)
/* Building library as dll */
#    define NTNDARRAYCONVERTER_API __declspec(dllexport)
#  elif !defined(BUILDING_ntndArrayConverter_API) && defined(EPICS_CALL_DLL)
/* Calling library in dll form */
#    define NTNDARRAYCONVERTER_API __declspec(dllimport)
#  endif

#elif __GNUC__ >= 4
#  define NTNDARRAYCONVERTER_API __attribute__ ((visibility("default")))
#endif

#if !defined(NTNDARRAYCONVERTER_API)
#  define NTNDARRAYCONVERTER_API
#endif

#if !defined(epicsStdCall)
#  define epicsStdCall
#endif

#endif /* INC_ntndArrayConverterAPI_H */

