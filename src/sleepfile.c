#include <nanoresource/nanoresource.h>
#include <ras/ras.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "sleepfile/sleepfile.h"

static void
sleepfile_resource_open(nanoresource_request_t *request);

static void
sleepfile_resource_close(nanoresource_request_t *request);

static void
sleepfile_resource_destroy(nanoresource_request_t *request);

sleepfile_t *
sleepfile_new(
  ras_storage_t *storage,
  struct sleepfile_options options
) {
  sleepfile_t *sleepfile = nanoresource_allocator_alloc(sizeof(*sleepfile));

  if (0 == sleepfile) {
    return 0;
  }

  if (0 == memset(sleepfile, 0, sizeof(*sleepfile))) {
    free(sleepfile);
    return 0;
  }

  sleepfile->magic_bytes = options.magic_bytes;
  sleepfile->value_codec = options.value_codec;
  sleepfile->value_size = options.value_size;
  sleepfile->version = 0;
  sleepfile->storage = storage;

  sleepfile->readable = 0;
  sleepfile->writable = 0;

  storage->data = sleepfile;

  memset(sleepfile->name,0, sizeof(sleepfile->name));

  if (0 != options.name) {
    memcpy(sleepfile->name, options.name, sizeof(sleepfile->name));
  }

  nanoresource_init((nanoresource_t *) sleepfile,
    (nanoresource_options_t) {
      .data = sleepfile,
      .open = sleepfile_resource_open,
      .close = sleepfile_resource_close,
      .destroy = sleepfile_resource_destroy
    });

  sleepfile->alloc = 1;

  return sleepfile;
}

/**
static void
write_header(sleepfile_t *sleepfile) {
  if (nanoresource_active((nanoresource_t *) sleepfile) > 0) {
    return;
  }

  printf("write_header\n");
}
*/

static void
parse_header(
  sleepfile_t *sleepfile,
  void *value,
  ras_request_t *request
) {
  nanoresource_request_t *req = request->shared;
  unsigned char *buffer = value;
  int magic_bytes =
      ((buffer[0] & 0xff) << 24)
    | ((buffer[1] & 0xff) << 16)
    | ((buffer[2] & 0xff) << 8)
    | ((buffer[3] & 0xff) << 0);

  int value_size =
      ((buffer[5] & 0xff) << 8)
    | ((buffer[6] & 0xff) << 0);

  if (0 != sleepfile->magic_bytes && magic_bytes != sleepfile->magic_bytes) {
    // @TODO use real error
    req->callback(req, -1);
    return;
  }

  if ((unsigned int) 0 != buffer[4]) {
    // @TODO use real error
    req->callback(req, -1);
    return;
  }

  if (0 != sleepfile->value_size && value_size != sleepfile->value_size) {
    // @TODO use real error
    req->callback(req, -1);
    return;
  }

  char name[buffer[7]];
  memset(name, 0, buffer[7]);
  memcpy(name, buffer + 8, buffer[7]);

  if (0 != strncmp(name, sleepfile->name, buffer[7])) {
    // @TODO use real error
    req->callback(req, -1);
    return;
  }

  sleepfile->magic_bytes = magic_bytes;
  sleepfile->value_size = value_size;
  sleepfile->writable = 1;
  memcpy(sleepfile->name, name, buffer[7]);

  req->callback(req, 0);
}

static int
sleepfile_open_storage_read_request(
  ras_request_t *request,
  int err,
  void *value,
  unsigned long int size
) {
  sleepfile_t *sleepfile = (sleepfile_t *) request->storage->data;
  nanoresource_request_t *req = request->shared;

  if (ENOMEM == err || ENOENT == err) {
    return req->callback(req, 0);
  } else if (err > 0) {
    return req->callback(req, err);
  }

  sleepfile->readable = 1;

  parse_header(sleepfile, value, request);
  return 0;
}

static int
sleepfile_open_storage_open_request(
  ras_request_t *request,
  int err,
  void *value,
  unsigned long int size
) {
  sleepfile_t *sleepfile = (sleepfile_t *) request->storage->data;
  ras_storage_read_shared(
    sleepfile->storage,
    0, // offset
    32, // size
    0, // noop callback
    sleepfile_open_storage_read_request,
    request->shared);
  return 0;
}

static void
sleepfile_resource_open(nanoresource_request_t *request) {
  sleepfile_t *sleepfile = (sleepfile_t *) request->resource;

  ras_storage_open_shared(
    sleepfile->storage,
    0, // noop callback
    sleepfile_open_storage_open_request,
    request);
}

int
sleepfile_open(
  sleepfile_t *sleepfile,
  sleepfile_open_callback_t *callback
) {
  return nanoresource_open(
    (nanoresource_t *) sleepfile,
    (nanoresource_open_callback_t *) callback);
}

static int
sleepfile_close_storage_close_request(
  ras_request_t *request,
  int err,
  void *value,
  unsigned long int size
) {
  nanoresource_request_t *req = request->shared;
  req->callback(req, err);
  return 0;
}

static void
sleepfile_resource_close(nanoresource_request_t *request) {
  sleepfile_t *sleepfile = (sleepfile_t *) request->resource;

  ras_storage_open_shared(
    sleepfile->storage,
    0, // noop callback
    sleepfile_close_storage_close_request,
    request);
}

int
sleepfile_close(
  sleepfile_t *sleepfile,
  sleepfile_close_callback_t *callback
) {
  return nanoresource_close(
    (nanoresource_t *) sleepfile,
    (nanoresource_close_callback_t *) callback);
}

static int
sleepfile_get_storage_read_request(
  ras_request_t *request,
  int err,
  void *value,
  unsigned long int size
) {
  sleepfile_t *sleepfile = request->storage->data;
  sleepfile_get_options_t *options = request->shared;
  sleepfile_get_callback_t *callback = 0;
  unsigned int index = 0;
  void *enc = 0;
  void *ctx = 0;

  if (0 != options) {
    callback = options->callback;
    index = options->index;
    ctx = options->ctx;
    request->shared = 0;
    free(options);
    options = 0;
  }

  if (0 != sleepfile->value_codec.decode) {
    enc = sleepfile->value_codec.decode(
      value,
      sleepfile->value_size,
      0,
      index);
  }

  nanoresource_inactive((nanoresource_t *) sleepfile);

  if (0 != callback) {
    callback(sleepfile, err, 0 != enc ? enc : value, ctx);
  }

  if (0 != enc) {
    free(enc);
  }

  return 0;
}

static int
sleepfile_get_storage_open_request(
  ras_request_t *request,
  int err,
  void *value,
  unsigned long int size
) {
  sleepfile_get_options_t *options = request->shared;
  sleepfile_t *sleepfile = request->storage->data;

  ras_storage_read_shared(
    request->storage,
    32 + options->index * sleepfile->value_size,
    sleepfile->value_size,
    0, // noop callback
    sleepfile_get_storage_read_request,
    options);
  return 0;
}

int
sleepfile_get(
  sleepfile_t *sleepfile,
  unsigned int index,
  sleepfile_get_callback_t *callback,
  void *ctx
) {
  int err = 0;

  if (1 != sleepfile->opened) {
    if ((err = sleepfile_open(sleepfile, 0) > 0)) {
      return err;
    }
  }

  if ((err = nanoresource_active((nanoresource_t *) sleepfile)) > 0) {
    return err;
  }

  sleepfile_get_options_t *options = malloc(sizeof(*options));
  *options = (sleepfile_get_options_t) { index, callback, ctx };

  return ras_storage_open_shared(
    sleepfile->storage,
    0, // noop callback
    sleepfile_get_storage_open_request,
    options);
}

static int
sleepfile_put_storage_write_request(
  ras_request_t *request,
  int err,
  void *value,
  unsigned long int size
) {
  sleepfile_t *sleepfile = request->storage->data;
  sleepfile_put_options_t *options = request->shared;
  sleepfile_put_callback_t *callback = 0;
  void *ctx = 0;

  if (0 == options) {
    return 1;
  }

  if (0 != options) {
    callback = options->callback;
    ctx = options->ctx;
    request->shared = 0;

    if (0 != sleepfile->value_codec.encode && 1 == options->encoded) {
      free(options->value);
    }

    free(options);
    options = 0;
  }

  nanoresource_inactive((nanoresource_t *) sleepfile);

  if (0 != callback) {
    callback(sleepfile, err, ctx);
  }

  return 0;
}

static int
sleepfile_put_storage_open_request(
  ras_request_t *request,
  int err,
  void *value,
  unsigned long int size
) {
  sleepfile_put_options_t *options = request->shared;
  sleepfile_t *sleepfile = request->storage->data;

  if (0 == sleepfile->writable) {
    // @TODO
  }

  if (0 != sleepfile->value_codec.encode) {
    options->encoded = 1;
    options->value = sleepfile->value_codec.encode(
      options->value,
      sleepfile->value_size,
      options->index);
  }

  ras_storage_write_shared(
    request->storage,
    32 + options->index * sleepfile->value_size,
    sleepfile->value_size,
    options->value,
    0, // noop callback
    sleepfile_put_storage_write_request,
    options);
  return 0;
}

int
sleepfile_put(
  sleepfile_t *sleepfile,
  unsigned int index,
  void *value,
  sleepfile_put_callback_t *callback,
  void *ctx
) {
  int err = 0;

  if (1 != sleepfile->opened) {
    if ((err = sleepfile_open(sleepfile, 0) > 0)) {
      return err;
    }
  }

  if ((err = nanoresource_active((nanoresource_t *) sleepfile)) > 0) {
    return err;
  }

  sleepfile_put_options_t *options = malloc(sizeof(*options));
  *options = (sleepfile_put_options_t) { index, value, callback, 0, ctx };

  return ras_storage_open_shared(
    sleepfile->storage,
    0, // noop callback
    sleepfile_put_storage_open_request,
    options);
}

static int
sleepfile_stat_storage_stat_request(
  ras_request_t *request,
  int err,
  void *value,
  unsigned long int size
) {
  sleepfile_t *sleepfile = request->storage->data;
  sleepfile_stat_options_t *options = request->shared;
  sleepfile_stat_callback_t *callback = 0;
  sleepfile_stats_t stats = { 0 };
  void *ctx = 0;

  if (0 == err) {
    sleepfile_storage_stats_t *stat = malloc(sizeof(*stat));

    memset(stat, 0, sizeof(*stat));
    memcpy(stat, value, size);

    stats.magic_bytes = sleepfile->magic_bytes;
    stats.value_size = sleepfile->value_size;
    stats.version = sleepfile->version;
    stats.name = sleepfile->name;

    stats.length = (unsigned long int) fmax(0.0,
      floorf((stat->size - 32) / sleepfile->value_size));

    if (stat->blocks >  0) {
      stats.density = (stat->blocks / 8.0) / ceil(stat->size/ 4096);
    } else {
      stats.density = 1;
    }

    free(stat);
  }

  if (0 != options) {
    callback = options->callback;
    ctx = options->ctx;
    request->shared = 0;
    free(options);
    options = 0;
  }

  nanoresource_inactive((nanoresource_t *) sleepfile);

  if (0 != callback) {
    callback(sleepfile, err, &stats, ctx);
  }

  return 0;
}

int
sleepfile_stat(
  sleepfile_t *sleepfile,
  sleepfile_stat_callback_t *callback,
  void *ctx
) {
  int err = 0;

  if ((err = nanoresource_active((nanoresource_t *) sleepfile)) > 0) {
    return err;
  }

  sleepfile_stat_options_t *options = malloc(sizeof(*options));
  *options = (sleepfile_stat_options_t) { callback, ctx };

  return ras_storage_stat_shared(
    sleepfile->storage,
    0, // noop callback
    sleepfile_stat_storage_stat_request,
    options);
  return 0;
}

static int
sleepfile_destroy_storage_destroy_request(
  ras_request_t *request,
  int err,
  void *value,
  unsigned long int size
) {
  nanoresource_request_t *req = request->shared;
  req->callback(req, err);
  return 0;
}

static void
sleepfile_resource_destroy(nanoresource_request_t *request) {
  sleepfile_t *sleepfile = (sleepfile_t *) request->resource;
  ras_storage_destroy_shared(
    sleepfile->storage,
    0, // noop callback
    sleepfile_destroy_storage_destroy_request,
    request);
}

int
sleepfile_destroy(
  sleepfile_t *sleepfile,
  sleepfile_destroy_callback_t *callback
) {
  return nanoresource_destroy(
    (nanoresource_t *) sleepfile,
    (nanoresource_destroy_callback_t *) callback);
}
