#include "sleepfile/version.h"

#define SX(x) #x
#define S(x) SX(x)

#ifndef SLEEPFILE_VERSION
#define SLEEPFILE_VERSION 0
#endif

#ifndef SLEEPFILE_NAME
#define SLEEPFILE_NAME "libsleepfile"
#endif

#ifndef SLEEPFILE_VERSION_MAJOR
#define SLEEPFILE_VERSION_MAJOR 0
#endif

#ifndef SLEEPFILE_VERSION_MINOR
#define SLEEPFILE_VERSION_MINOR 0
#endif

#ifndef SLEEPFILE_VERSION_PATCH
#define SLEEPFILE_VERSION_PATCH 0
#endif

#ifndef SLEEPFILE_VERSION_REVISION
#define SLEEPFILE_VERSION_REVISION 0
#endif

#ifndef SLEEPFILE_DATE_COMPILED
#define SLEEPFILE_DATE_COMPILED ""
#endif

const char *
sleepfile_version_string() {
  return SLEEPFILE_NAME
    "@" S(SLEEPFILE_VERSION_MAJOR)
    "." S(SLEEPFILE_VERSION_MINOR)
    "." S(SLEEPFILE_VERSION_PATCH)
    "." S(SLEEPFILE_VERSION_REVISION) " (" SLEEPFILE_DATE_COMPILED ")";
}

const unsigned long int
sleepfile_version() {
  return (const unsigned long int) SLEEPFILE_VERSION;
}

const unsigned long int
sleepfile_version_major() {
  return SLEEPFILE_VERSION >> 24 & 0xff;
}

const unsigned long int
sleepfile_version_minor() {
  return SLEEPFILE_VERSION >> 16 & 0xff;
}

const unsigned long int
sleepfile_version_patch() {
  return SLEEPFILE_VERSION >> 8 & 0xff;
}

const unsigned long int
sleepfile_version_revision() {
  return SLEEPFILE_VERSION & 0xff;
}
