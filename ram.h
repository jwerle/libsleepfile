#ifndef SLEEPFILE_RAM_H
#define SLEEPFILE_RAM_H

#include <ras/ras.h>

#ifndef SLEEPFILE_RAM_MAX_BUFFERS
#define SLEEPFILE_RAM_MAX_BUFFERS 1024
#endif

typedef struct ram_s ram_t;
struct ram_s {
  RAS_STORAGE_FIELDS;
  unsigned long int length;
  unsigned int page_size;
  unsigned char *buffers[SLEEPFILE_RAM_MAX_BUFFERS];
};

int
ram_destroy(
  ras_storage_t *ram,
  ras_storage_destroy_callback_t *callback);

int
ram_stat(
  ras_storage_t *ram,
  ras_storage_stat_callback_t *callback);

int
ram_delete(
  ras_storage_t *ram,
  unsigned long int offset,
  unsigned long int size,
  ras_storage_delete_callback_t *callback
);

int
ram_read(
  ras_storage_t *ram,
  unsigned long int offset,
  unsigned long int size,
  ras_storage_read_callback_t *callback
);

int
ram_write(
  ras_storage_t *ram,
  unsigned long int offset,
  unsigned long int size,
  void *buffer,
  ras_storage_write_callback_t *callback
);

ram_t *
ram_alloc();

ras_storage_t *
ram_new();

#endif
