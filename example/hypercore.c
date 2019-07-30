#include <sleepfile/sleepfile.h>
#include <ram/ram.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define UINT_32_MAX ((int) pow(2.0, 32.0))

typedef struct hypercore hypercore_t;
typedef struct hypercore_node hypercore_node_t;

struct hypercore {
  struct {
    ras_storage_t *signatures;
    ras_storage_t *tree;
    ras_storage_t *data;
  } storage;

  struct {
    sleepfile_t *tree; // type=1
    sleepfile_t *signatures; // type=2
  } sleep;
};

struct hypercore_node {
  unsigned long int index;
  unsigned long int size;
  unsigned char hash[32];
};

void *
decode_node(
  void *value,
  unsigned long int size,
  unsigned long int offset,
  unsigned long int index
) {
  hypercore_node_t *node = malloc(sizeof(*node));
  unsigned char *buffer = value;
  unsigned int *len = value;

  memset(node, 0, sizeof(*node));
  memcpy(node->hash, value, 32);

  node->index = index;

  int top =
    ((buffer[32 + 0] & 0xff) << 0)
  | ((buffer[32 + 1] & 0xff) << 8)
  | ((buffer[32 + 2] & 0xff) << 16)
  | ((buffer[32 + 3] & 0xff) << 24);

  int rem =
    ((buffer[32 + 4] & 0xff) * (int) pow(2, 24))
  | ((buffer[32 + 5] & 0xff) * (int) pow(2, 16))
  | ((buffer[32 + 6] & 0xff) * (int) pow(2, 8))
  | ((buffer[32 + 7] & 0xff));

  node->size = top * UINT_32_MAX + rem;

  return node;
}

void
onnode(sleepfile_t *sleepfile, int err, void *value) {
  hypercore_node_t *node = value;
  char hash_string[64 + 32];

  for (int i = 0, j = 0; i < 32; ++i, j+=3) {
    sprintf(hash_string + j, "%0.2x", node->hash[i] & 0xff);
    if (i + 1 != 32) {
      sprintf(hash_string + j + 2, "  ");
    }
  }

  printf(
    "Node {\n"
    "  index=%lu\n"
    "  size=%lu\n"
    "  hash[]={%s}\n"
    "}\n",
    node->index,
    node->size,
    hash_string
  );
}

int
main(void) {
  hypercore_t feed = { 0 };
  unsigned char tree[] = {
    0x05, 0x02, 0x57, 0x02, 0x00, 0x00, 0x28, 0x07,
    0x42, 0x4c, 0x41, 0x4b, 0x45, 0x32, 0x62, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x67, 0x17, 0xb2, 0x5f, 0x24, 0xd9, 0x6c, 0xcb,
    0xc9, 0x51, 0x66, 0xba, 0xcb, 0xb6, 0x71, 0xd5,
    0x9e, 0xb4, 0x26, 0x3e, 0xe5, 0xe1, 0xaa, 0x0f,
    0x6b, 0x15, 0x20, 0x81, 0x5c, 0xbe, 0xe8, 0x0b,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05
  };

  unsigned char signatures[] = {
    0x05, 0x02, 0x57, 0x01, 0x00, 0x00, 0x40, 0x07,
    0x45, 0x64, 0x32, 0x35, 0x35, 0x31, 0x39, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x97, 0x1e, 0xad, 0x95, 0x60, 0x3e, 0xe0, 0x17,
    0xda, 0x47, 0x8c, 0x7b, 0x5a, 0xf8, 0xcd, 0xb7,
    0xf5, 0xc8, 0x4d, 0xb8, 0xcc, 0x40, 0x89, 0xa7,
    0x8b, 0x44, 0x90, 0xbc, 0xb4, 0x63, 0x22, 0x6a,
    0x19, 0xa8, 0x4e, 0x9f, 0x63, 0xdc, 0xb1, 0x8f,
    0x7f, 0xa0, 0x6b, 0x81, 0xea, 0xc2, 0xae, 0xcb,
    0x75, 0x1e, 0xd3, 0x0d, 0xd5, 0xbc, 0x1a, 0x8d,
    0x8d, 0x63, 0xcf, 0xc0, 0xd7, 0x9c, 0x88, 0x0a
  };

  unsigned char data[] = {
    0x68, 0x65, 0x6c, 0x6c, 0x6f
  };

  feed.storage.data = ram_new();
  feed.storage.tree = ram_new();
  feed.storage.signatures = ram_new();

  ram_write(feed.storage.data, 0, sizeof(data), data, 0);
  ram_write(feed.storage.tree, 0, sizeof(tree), tree, 0);
  ram_write(feed.storage.signatures, 0, sizeof(signatures), signatures, 0);

  feed.sleep.signatures = sleepfile_new(feed.storage.signatures,
    (sleepfile_options_t) {
      .magic_bytes = 0x05025701,
      .value_size = 64,
      .name = "Ed25519"
    });

  feed.sleep.tree = sleepfile_new(feed.storage.tree,
    (sleepfile_options_t) {
      .magic_bytes = 0x05025702,
      .value_codec = (sleepfile_codec_t) { 0, decode_node },
      .value_size = 40,
      .name = "BLAKE2b"
    });

  sleepfile_get(feed.sleep.tree, 0, onnode);

  return 0;
}
