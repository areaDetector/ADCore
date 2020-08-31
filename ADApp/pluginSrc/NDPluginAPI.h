/* This is a generated file, do not edit! */

#ifndef INC_NDPluginAPI_H
#define INC_NDPluginAPI_H

#if defined(_WIN32) || defined(__CYGWIN__)

#  if !defined(epicsStdCall)
#    define epicsStdCall __stdcall
#  endif

#  if defined(BUILDING_NDPlugin_API) && defined(EPICS_BUILD_DLL)
/* Building library as dll */
#    define NDPLUGIN_API __declspec(dllexport)
#  elif !defined(BUILDING_NDPlugin_API) && defined(EPICS_CALL_DLL)
/* Calling library in dll form */
#    define NDPLUGIN_API __declspec(dllimport)
#  endif

#elif __GNUC__ >= 4
#  define NDPLUGIN_API __attribute__ ((visibility("default")))
#endif

#if !defined(NDPLUGIN_API)
#  define NDPLUGIN_API
#endif

#if !defined(epicsStdCall)
#  define epicsStdCall
#endif

#endif /* INC_NDPluginAPI_H */

