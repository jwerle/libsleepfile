#ifndef SLEEPFILE_VERSION_H
#define SLEEPFILE_VERSION_H

#include "platform.h"

/**
 * Returns the version string for the library.
 */
SLEEPFILE_EXPORT const char *
sleepfile_version_string();

/**
 * Returns the 32 bit version number that encodes the
 * major, minor, patch, and revision version parts.
 */
SLEEPFILE_EXPORT const unsigned long int
sleepfile_version();

/**
 * Returns the major version part.
 */
SLEEPFILE_EXPORT const unsigned long int
sleepfile_version_major();

/**
 * Returns the minor version part.
 */
SLEEPFILE_EXPORT const unsigned long int
sleepfile_version_minor();

/**
 * Returns the minor patch part.
 */
SLEEPFILE_EXPORT const unsigned long int
sleepfile_version_patch();

/**
 * Returns the minor revision part.
 */
SLEEPFILE_EXPORT const unsigned long int
sleepfile_version_revision();

#endif
