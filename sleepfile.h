#ifndef SLEEPFILE_H
#define SLEEPFILE_H

#include <nanoresource/nanoresource.h>
#include <ras/ras.h>

typedef struct sleepfile sleepfile_t;
typedef struct sleepfile_codec sleepfile_codec_t;
typedef struct sleepfile_stats sleepfile_stats_t;
typedef struct sleepfile_options sleepfile_options_t;
typedef struct sleepfile_storage_stats sleepfile_storage_stats_t;

typedef struct sleepfile_get_options sleepfile_get_options_t;
typedef struct sleepfile_put_options sleepfile_put_options_t;
typedef struct sleepfile_stat_options sleepfile_stat_options_t;

typedef void (sleepfile_open_callback_t)(sleepfile_t *sleepfile, int err);
typedef void (sleepfile_close_callback_t)(sleepfile_t *sleepfile, int err);
typedef void (sleepfile_destroy_callback_t)(sleepfile_t *sleepfile, int err);

typedef void (sleepfile_put_callback_t)(sleepfile_t *sleepfile, int err);
typedef void (sleepfile_get_callback_t)(
  sleepfile_t *sleepfile,
  int err,
  void *value);

typedef void (sleepfile_stat_callback_t)(
  sleepfile_t *sleepfile,
  int err,
  sleepfile_stats_t *stats);

struct sleepfile_codec {
  void *(*encode)(void *value, unsigned long int size);
  void *(*decode)(void *buf, unsigned long int size, unsigned long int offset);
};

struct sleepfile_options {
  ras_storage_t *storage;
  sleepfile_codec_t value_codec;
  unsigned long int value_size;
  unsigned long int magic_bytes;
  char *name;
};

struct sleepfile {
  NANORESOURCE_FIELDS;
  ras_storage_t *storage;
  sleepfile_codec_t value_codec;
  unsigned long int value_size;
  unsigned long int magic_bytes;
  unsigned long int version;
  char *name;
  unsigned int readable:1;
  unsigned int writable:1;
};

struct sleepfile_stats {
  unsigned long int value_size;
  unsigned long int magic_bytes;
  unsigned long int version;
  unsigned long int length;
  long double density;
  char *name;
};

struct sleepfile_storage_stats {
  RAS_STORAGE_STATS_FIELDS;
  unsigned long int blocks;
};

struct sleepfile_stat_options {
  sleepfile_stat_callback_t *callback;
};

struct sleepfile_get_options {
  unsigned int index;
  sleepfile_get_callback_t *callback;
};

struct sleepfile_put_options {
  unsigned int index;
  void *value;
  sleepfile_put_callback_t *callback;
  unsigned int encoded:1;
};

sleepfile_t *
sleepfile_new(
  ras_storage_t *storage,
  sleepfile_options_t options);

int
sleepfile_destroy(
  sleepfile_t *sleepfile,
  sleepfile_destroy_callback_t *callback);

int
sleepfile_open(
  sleepfile_t *sleepfile,
  sleepfile_open_callback_t *callback);

int
sleepfile_close(
  sleepfile_t *sleepfile,
  sleepfile_close_callback_t *callback);

int
sleepfile_stat(
  sleepfile_t *sleepfile,
  sleepfile_stat_callback_t *callback);

int
sleepfile_get(
  sleepfile_t *sleepfile,
  unsigned int index,
  sleepfile_get_callback_t *callback);

int
sleepfile_put(
  sleepfile_t *sleepfile,
  unsigned int index,
  void *value,
  sleepfile_put_callback_t *callback);

#endif
