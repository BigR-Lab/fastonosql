#ifndef __HIREDIS_FMACRO_H
#define __HIREDIS_FMACRO_H

#if defined(__linux__)
#define _BSD_SOURCE
#define _DEFAULT_SOURCE
#endif

#if defined(__CYGWIN__)
#include <sys/cdefs.h>
#endif

#ifdef FASTO
  #if defined(__sun__)
  #define _POSIX_C_SOURCE 200112L
  #elif defined(__linux__) || defined(__OpenBSD__) || defined(__NetBSD__)
  #define _XOPEN_SOURCE 600
  #else
  #define _XOPEN_SOURCE
  #endif
#else
  #if defined(__sun__)
  #define _POSIX_C_SOURCE 200112L
  #else
  #define _XOPEN_SOURCE 600
  #endif
#endif

#if defined(__APPLE__) && defined(__MACH__)
#define _OSX
#endif

#endif
