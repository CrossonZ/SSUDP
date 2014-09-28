#ifndef __INCLUDED_UDP_THREAD_H_
#define __INCLUDED_UDP_THREAD_H_

#include "GlobalDefines.h"
#include <process.h>

#if defined(_WIN32)
#define UDP_THREAD_DECLARATION(functionName) unsigned __stdcall functionName( void* arguments )


#else
#define UDP_THREAD_DECLARATION(functionName) void* functionName( void* arguments )
#endif

class CUDPThread
{
public:
#if defined(_WIN32)
static int Create( unsigned __stdcall start_address( void* ), void *arglist, int priority=0);

#else
static int Create( void* start_address( void* ), void *arglist, int priority=0);
#endif
};

#endif
