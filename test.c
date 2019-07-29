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
onstat(sleepfile_t *sleepfile, int err, sleepfile_stats_t *stats) {
  printf("onstat(err=%s)\n", strerror(err));

  if (0 == err) {
    printf(
    "Stats {\n"
    "  value_size=%lu\n"
    "  magic_bytes=0x%0.8lx\n"
    "  version=%lu\n"
    "  length=%lu\n"
    "  density=%LG\n"
    "  name=%s\n"
    "}\n",
    stats->value_size,
    stats->magic_bytes,
    stats->version,
    stats->length,
    stats->density,
    stats->name);
  }

  sleepfile_destroy(sleepfile, ondestroy);
}

static void
onget(sleepfile_t *sleepfile, int err, void *value) {
  printf("onget(err=%s)\n", strerror(err));

  if (0 == err) {
    printf("%s\n", value);
  }

  sleepfile_stat(sleepfile, onstat);
}

static void
onput(sleepfile_t *sleepfile, int err) {
  printf("onput(err=%s)\n", strerror(err));
  sleepfile_get(sleepfile, 10, onget);
}

static void
onopen(sleepfile_t *sleepfile, int err) {
  printf("onopen(err=%s)\n", strerror(err));
  char *buffer = "hello";
  sleepfile_put(sleepfile, 10, buffer, onput);
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
      // [bitfield]=0x05025700, Ed25519=0x05025701, BLAKE2b=0x05025702
      .magic_bytes = 0x05025701,
      .value_codec = (sleepfile_codec_t) { encode, decode },
      .value_size = 64,
      .name = "Ed25519"
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
