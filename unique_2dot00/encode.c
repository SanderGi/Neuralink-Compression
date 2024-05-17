#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wav.h"

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Usage: %s <filename.wav> <filename.brainwire>", argv[0]);
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

  // only handle one sub-format for simplicity:
  if (header.format_type != 1) {
    printf("Only PCM format supported\n");
    exit(1);
  }
  if (header.channels != 1) {
    printf("Only 1 channel supported\n");
    exit(1);
  }
  if (size_of_each_sample != 2) {
    printf("Only 2 byte samples supported\n");
    exit(1);
  }

  short unique_values[num_samples];
  int num_unique_values = 0;
  short datas[num_samples];

  unsigned char data_buffer[size_of_each_sample];
  for (long i = 0; i < num_samples; i++) {
    if (fread(data_buffer, sizeof(data_buffer), 1, src) != 1) {
      printf("Error reading file\n");
      exit(1);
    }

    short data = read_data_sample(data_buffer, size_of_each_sample);

    int is_unique = TRUE;
    for (int j = 0; j < num_unique_values; j++) {
      if (unique_values[j] == data) {
        is_unique = FALSE;
        datas[i] = j;
        break;
      }
    }
    if (is_unique) {
      unique_values[num_unique_values] = data;
      datas[i] = num_unique_values;
      num_unique_values++;
    }
  }

  // write the raw header
  if (fwrite(raw_header, RAW_HEADER_SIZE, 1, dst) != 1) {
    printf("Error writing file\n");
    exit(1);
  }

  // write number of unique values and the unique values
  if (DEBUG) printf("Number of unique values: %d\n", num_unique_values);
  if (fwrite(&num_unique_values, sizeof(num_unique_values), 1, dst) != 1) {
    printf("Error writing file\n");
    exit(1);
  }
  if (fwrite(unique_values, sizeof(short) * num_unique_values, 1, dst) != 1) {
    printf("Error writing file\n");
    exit(1);
  }

  // write the data
  if (DEBUG) printf("Number of samples: %ld\n", num_samples);
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
  struct BitWriter *writer = BitWriter_alloc(dst);
  for (long i = 0; i < num_samples; i++) {
    BitWriter_write(writer, datas[i], bits_per_sample);
  }
  BitWriter_flush(writer);
  BitWriter_free(writer);

  fclose(src);
  fclose(dst);
  return 0;
}
