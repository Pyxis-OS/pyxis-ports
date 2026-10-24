#include <zip.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const char payload[] =
  "Public libzip deflate roundtrip. Public libzip deflate roundtrip. "
  "Public libzip deflate roundtrip. Public libzip deflate roundtrip.\n";

static int add_public_entry(zip_t *archive, bool explicit_time)
{
  zip_source_t *entry = zip_source_buffer(archive, payload, sizeof(payload) - 1, 0);
  if (!entry) {
    return -1;
  }
  zip_int64_t index = zip_file_add(archive, "public.txt", entry, ZIP_FL_ENC_UTF_8);
  if (index < 0) {
    zip_source_free(entry);
    return -1;
  }
  if (zip_file_set_encryption(archive, (zip_uint64_t)index, ZIP_EM_TRAD_PKWARE, "unsupported") != -1 ||
      zip_error_code_zip(zip_get_error(archive)) != ZIP_ER_ENCRNOTSUPP) {
    fprintf(stderr, "PKWARE encryption was not rejected as unsupported\n");
    return -1;
  }
  zip_error_clear(archive);
  if (zip_set_file_compression(archive, (zip_uint64_t)index, ZIP_CM_BZIP2, 0) != -1 ||
      zip_error_code_zip(zip_get_error(archive)) != ZIP_ER_COMPNOTSUPP) {
    fprintf(stderr, "BZip2 compression was not rejected as unsupported\n");
    return -1;
  }
  zip_error_clear(archive);
  if (zip_set_file_compression(archive, (zip_uint64_t)index, ZIP_CM_DEFLATE, 6) < 0) {
    return -1;
  }
  if (explicit_time) {
    /* Authored ZIP date: 2026-10-24, 00:00:00. No current-time observation. */
    zip_uint16_t date = (zip_uint16_t)(((2026 - 1980) << 9) | (10 << 5) | 24);
    if (zip_file_set_dostime(archive, (zip_uint64_t)index, 0, date, 0) < 0) {
      return -1;
    }
  }
  return 0;
}

static int verify_public_entry(zip_t *archive)
{
  zip_stat_t stat;
  if (zip_stat(archive, "public.txt", 0, &stat) < 0 ||
      !(stat.valid & ZIP_STAT_COMP_METHOD) || stat.comp_method != ZIP_CM_DEFLATE ||
      !(stat.valid & ZIP_STAT_SIZE) || stat.size != sizeof(payload) - 1) {
    fprintf(stderr, "deflate entry metadata differs from the authored payload\n");
    return -1;
  }
  zip_file_t *file = zip_fopen(archive, "public.txt", 0);
  if (!file) {
    return -1;
  }
  char output[sizeof(payload)];
  zip_int64_t count = zip_fread(file, output, sizeof(output));
  char extra;
  bool valid = count == (zip_int64_t)(sizeof(payload) - 1) &&
    memcmp(output, payload, sizeof(payload) - 1) == 0 && zip_fread(file, &extra, 1) == 0;
  if (zip_fclose(file) < 0) {
    valid = false;
  }
  if (!valid) {
    fprintf(stderr, "deflate contents differ from the authored payload\n");
    return -1;
  }
  printf("public deflate entry: %llu bytes; timestamp %s\n",
    (unsigned long long)stat.size, (stat.valid & ZIP_STAT_MTIME) ? "known" : "unknown");
  return 0;
}

static int memory_roundtrip(void)
{
  zip_error_t error;
  zip_error_init(&error);
  zip_t *archive = NULL;
  zip_source_t *source = zip_source_buffer_create(NULL, 0, 0, &error);
  int result = -1;
  if (!source) {
    goto done;
  }
  zip_stat_t metadata;
  if (zip_source_stat(source, &metadata) < 0 || (metadata.valid & ZIP_STAT_MTIME)) {
    fprintf(stderr, "memory source unexpectedly claims an intrinsic timestamp\n");
    goto done;
  }
  archive = zip_open_from_source(source, ZIP_CREATE | ZIP_TRUNCATE, &error);
  if (!archive) {
    goto done;
  }
  zip_source_keep(source);
  if (add_public_entry(archive, true) < 0 || zip_close(archive) < 0) {
    fprintf(stderr, "memory write: %s\n", zip_strerror(archive));
    zip_discard(archive);
    archive = NULL;
    goto done;
  }
  archive = zip_open_from_source(source, ZIP_RDONLY, &error);
  if (!archive) {
    goto done;
  }
  source = NULL;
  if (verify_public_entry(archive) < 0) {
    goto done;
  }
  result = 0;
done:
  if (result < 0) {
    fprintf(stderr, "memory ZIP: %s\n", archive ? zip_strerror(archive) : zip_error_strerror(&error));
  }
  if (archive) {
    zip_discard(archive);
  }
  if (source) {
    zip_source_free(source);
  }
  zip_error_fini(&error);
  return result;
}

static int invalid_archive(void)
{
  static const char invalid[] = "Public non-ZIP input";
  zip_error_t error;
  zip_error_init(&error);
  zip_source_t *source = zip_source_buffer_create(invalid, sizeof(invalid) - 1, 0, &error);
  if (!source) {
    fprintf(stderr, "invalid-input source: %s\n", zip_error_strerror(&error));
    zip_error_fini(&error);
    return -1;
  }
  zip_t *archive = zip_open_from_source(source, ZIP_RDONLY, &error);
  bool rejected = !archive && zip_error_code_zip(&error) == ZIP_ER_NOZIP;
  if (archive) {
    zip_discard(archive);
  }
  else {
    zip_source_free(source);
  }
  printf("invalid archive: %s\n", zip_error_strerror(&error));
  zip_error_fini(&error);
  return rejected ? 0 : -1;
}

static int file_roundtrip(const char *path)
{
  int error;
  zip_t *archive = zip_open(path, ZIP_CREATE | ZIP_TRUNCATE, &error);
  if (!archive) {
    fprintf(stderr, "native archive open error: %d\n", error);
    return -1;
  }
  if (add_public_entry(archive, false) < 0 || zip_close(archive) < 0) {
    fprintf(stderr, "native ZIP write: %s\n", zip_strerror(archive));
    zip_discard(archive);
    return -1;
  }
  archive = zip_open(path, ZIP_RDONLY, &error);
  if (!archive) {
    fprintf(stderr, "native archive reopen error: %d\n", error);
    return -1;
  }
  int result = verify_public_entry(archive);
  zip_discard(archive);
  return result;
}

int main(int argc, char **argv)
{
  if (argc > 2) {
    fprintf(stderr, "usage: libzip-manual [PUBLIC_SCRATCH_ZIP_PATH]\n");
    return 2;
  }
  if (!zip_compression_method_supported(ZIP_CM_STORE, 0) ||
      !zip_compression_method_supported(ZIP_CM_STORE, 1) ||
      !zip_compression_method_supported(ZIP_CM_DEFLATE, 0) ||
      !zip_compression_method_supported(ZIP_CM_DEFLATE, 1) ||
      zip_encryption_method_supported(ZIP_EM_TRAD_PKWARE, 0) ||
      zip_encryption_method_supported(ZIP_EM_AES_256, 1)) {
    fprintf(stderr, "selected compression/encryption capabilities differ\n");
    return 1;
  }
  if (memory_roundtrip() < 0 || invalid_archive() < 0 ||
      (argc == 2 && file_roundtrip(argv[1]) < 0)) {
    return 1;
  }
  puts("public memory ZIP deflate/error checks completed; native file checked if selected");
  return 0;
}
