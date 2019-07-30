#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>

#include "sleepfile/sleepfile.h"
#include <ram/ram.h>
#include <ok/ok.h>

static void
ondestroy(sleepfile_t *sleepfile, int err) {
  assert(0 == err);
  ok("on destroy");
}

static void
onstat(sleepfile_t *sleepfile, int err, sleepfile_stats_t *stats, void *ctx) {
  assert(0 == err);
  ok("on stat");

  if (0 == err) {
    printf(
    "Stats {\n"
    "  value_size=%lu\n"
    "  magic_bytes=0x%08lx\n"
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

  assert(0x05025701 == stats->magic_bytes);
  assert(64 == stats->value_size);
  assert(0 == stats->version);
  assert(11 == stats->length);
  assert(1 == stats->density); // ram
  assert(0 == strcmp("Ed25519", stats->name));
  sleepfile_destroy(sleepfile, ondestroy);
}

static void
onget(sleepfile_t *sleepfile, int err, void *value, void *ctx) {
  assert(0 == err);
  assert(0 == strcmp("hello", (char *) value));
  ok("on get");
  sleepfile_stat(sleepfile, onstat, 0);
}

static void
onput(sleepfile_t *sleepfile, int err, void *ctx) {
  ok("on put");
  sleepfile_get(sleepfile, 10, onget, 0);
}

static void
onopen(sleepfile_t *sleepfile, int err) {
  ok("on open");
  char *buffer = "hello";
  sleepfile_put(sleepfile, 10, buffer, onput, 0);
}

static void *
encode(
  void *value,
  unsigned long int size,
  unsigned long int index
) {
  unsigned char *buffer = malloc(size);
  unsigned long int len = strlen(value);
  memset(buffer, 0, size);
  buffer[0] = len;
  memcpy(buffer + 1, value, size - 1);
  return buffer;
}

static void *
decode(
  void *value,
  unsigned long int size,
  unsigned long int offset,
  unsigned long int index
) {
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

  if (0 == sleepfile_open(sleepfile, onopen)) {
    ok("sleepfile open");
  }

  assert(
      nanoresource_allocator_stats().alloc
   <= nanoresource_allocator_stats().free);
  ok("nanoresource allocator");

  assert(
      ras_allocator_stats().alloc
   <= ras_allocator_stats().free);
  ok("ras allocator");

  ok_done();
  return 0;
}
