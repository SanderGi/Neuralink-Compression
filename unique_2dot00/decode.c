#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wav.h"

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Usage: %s <filename.brainwire> <filename.wav.copy>", argv[0]);
    exit(1);
  }

  char *src_name = argv[1];
  char *dst_name = argv[2];

  // open file
  FILE *src = fopen(src_name, "rb");
  if (src == NULL) {
    exit(1);
  }
  FILE *dst = fopen(dst_name, "wb");
  if (dst == NULL) {
    exit(1);
  }

  // read header
  unsigned char raw_header[RAW_HEADER_SIZE];
  long num_samples;
  long size_of_each_sample;
  struct wav_header header =
      parse_header(src, raw_header, &num_samples, &size_of_each_sample);

  // copy the raw header
  if (fwrite(raw_header, RAW_HEADER_SIZE, 1, dst) != 1) {
    printf("Error writing file\n");
    exit(1);
  }

  // read number of unique values and the unique values
  int num_unique_values;
  if (fread(&num_unique_values, sizeof(int), 1, src) != 1) {
    printf("Error reading file\n");
    exit(1);
  }
  short unique_values[num_unique_values];
  if (fread(unique_values, sizeof(short) * num_unique_values, 1, src) != 1) {
    printf("Error reading file\n");
    exit(1);
  }
  if (DEBUG) printf("Number of unique values: %d\n", num_unique_values);

  // read the data samples
  if (DEBUG) printf("Number of samples: %ld\n", num_samples);
  short datas[num_samples];
  int bits_per_sample = (num_unique_values < 64)      ? 6
                        : (num_unique_values < 128)   ? 7
                        : (num_unique_values < 256)   ? 8
                        : (num_unique_values < 512)   ? 9
                        : (num_unique_values < 1024)  ? 10
                        : (num_unique_values < 2048)  ? 11
                        : (num_unique_values < 4096)  ? 12
                        : (num_unique_values < 8192)  ? 13
                        : (num_unique_values < 16384) ? 14
                        : (num_unique_values < 32768) ? 15
                                                      : 16;
  if (DEBUG) printf("Bits per sample: %d\n", bits_per_sample);
  struct BitReader *reader = BitReader_alloc(src);
  for (long i = 0; i < num_samples; i++) {
    datas[i] = BitReader_read(reader, bits_per_sample);
  }
  BitReader_free(reader);

  // write the data samples
  for (long i = 0; i < num_samples; i++) {
    if (fwrite(&unique_values[datas[i]], sizeof(short), 1, dst) != 1) {
      printf("Error writing file\n");
      exit(1);
    }
  }

  fclose(src);
  fclose(dst);
  return 0;
}
