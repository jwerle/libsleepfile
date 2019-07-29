#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "sleepfile.h"
#include "ram.h"

static void
ondestroy(sleepfile_t *sleepfile, int err) {
  printf("ondestroy(err=%s)\n", strerror(err));
}

static void
onget(sleepfile_t *sleepfile, int err, void *value) {
  printf("onget(err=%s)\n", strerror(err));
  if (0 == err) {
    printf("%s\n", value);
  }
  sleepfile_destroy(sleepfile, ondestroy);
}

static void
onput(sleepfile_t *sleepfile, int err) {
  printf("onput(err=%s)\n", strerror(err));
  sleepfile_get(sleepfile, 1, onget);
}

static void
onopen(sleepfile_t *sleepfile, int err) {
  printf("onopen(err=%s)\n", strerror(err));
  char *buffer = "hello";
  sleepfile_put(sleepfile, 1, buffer, onput);
}

static void *
encode(void *value, unsigned long int size) {
  unsigned char *buffer = malloc(size);
  unsigned long int len = strlen(value);
  memset(buffer, 0, size);
  buffer[0] = len;
  memcpy(buffer + 1, value, size - 1);
  return buffer;
}

static void *
decode(void *value, unsigned long int size, unsigned long int offset) {
  unsigned char *buffer = (unsigned char *)value;
  unsigned long int len = buffer[0] + 1;
  unsigned char *dec = malloc(len);
  memset(dec, 0, len);
  memcpy(dec, buffer + 1, len);
  return dec;
}

int
main(void) {
  ras_storage_t *ram = ram_new();
  sleepfile_t *sleepfile = sleepfile_new(ram,
    (sleepfile_options_t) {
      .magic_bytes = 0x44448888,
      .value_codec = (sleepfile_codec_t) { encode, decode },
      .value_size = 32
    });

  sleepfile_open(sleepfile, onopen);

  printf("nanoresource_allocator_stats(%d, %d)\n",
      nanoresource_allocator_stats().alloc,
      nanoresource_allocator_stats().free);

  printf("ras_allocator_stats(%d, %d)\n",
      ras_allocator_stats().alloc,
      ras_allocator_stats().free);

  assert(
      nanoresource_allocator_stats().alloc
   <= nanoresource_allocator_stats().free);

  assert(
      ras_allocator_stats().alloc
   <= ras_allocator_stats().free);

  return 0;
}
