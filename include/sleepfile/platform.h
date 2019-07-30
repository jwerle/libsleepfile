#ifndef SLEEPFILE_PLATFORM_H
#define SLEEPFILE_PLATFORM_H

#if defined(_WIN32)
#  define SLEEPFILE_EXPORT __declspec(dllimport)
#elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR) >= 303
#  define SLEEPFILE_EXPORT __attribute__((visibility("default")))
#  define SLEEPFILE_INLINE inline
#else
#  define SLEEPFILE_EXPORT
#  define SLEEPFILE_INLINE
#endif

#ifndef SLEEPFILE_ALIGNMENT
#  define SLEEPFILE_ALIGNMENT sizeof(unsigned long) // platform word
#endif

#ifndef SLEEPFILE_MAX_ENUM
#  define SLEEPFILE_MAX_ENUM 0x7FFFFFFF
#endif

#endif
