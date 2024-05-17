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
  short freq_counts[num_samples];
  int num_unique_values = 0;
  short datas[num_samples];

  unsigned char data_buffer[size_of_each_sample];
  for (long i = 0; i < num_samples; i++) {
    if (fread(data_buffer, sizeof(data_buffer), 1, src) != 1) {
      printf("Error reading file\n");
      exit(1);
    }

    short data = read_data_sample(data_buffer, size_of_each_sample);
    datas[i] = data;

    int is_unique = TRUE;
    for (int j = 0; j < num_unique_values; j++) {
      if (unique_values[j] == data) {
        freq_counts[j]++;
        is_unique = FALSE;
        break;
      }
    }
    if (is_unique) {
      unique_values[num_unique_values] = data;
      freq_counts[num_unique_values] = 1;
      num_unique_values++;
    }
  }

  if (num_unique_values > 512) {
    execvp("./unique_2dot00/encode", argv);
  }

  // sort the unique values by frequency ascending
  for (int i = 0; i < num_unique_values; i++) {
    for (int j = i + 1; j < num_unique_values; j++) {
      if (freq_counts[j] < freq_counts[i]) {
        short tmp = unique_values[i];
        unique_values[i] = unique_values[j];
        unique_values[j] = tmp;
        tmp = freq_counts[i];
        freq_counts[i] = freq_counts[j];
        freq_counts[j] = tmp;
      }
    }
  }

  // construct Huffman tree
  struct HuffmanNode *root =
      Huffman_root(unique_values, freq_counts, num_unique_values);
  char *num_bits;
  long *codes =
      Huffman_codes(root, unique_values, num_unique_values, &num_bits);

  // write the raw header
  if (fwrite(raw_header, RAW_HEADER_SIZE, 1, dst) != 1) {
    printf("Error writing file\n");
    exit(1);
  }

  // write number of unique values
  if (fwrite(&num_unique_values, sizeof(num_unique_values), 1, dst) != 1) {
    printf("Error writing file\n");
    exit(1);
  }

  // write the huffman tree
  Huffman_save(dst, root);

  // write the data
  if (DEBUG) printf("Number of samples: %ld\n", num_samples);
  struct BitWriter *writer = BitWriter_alloc(dst);
  for (long i = 0; i < num_samples; i++) {
    short data = datas[i];
    for (int j = 0; j < num_unique_values; j++) {
      if (unique_values[j] == data) {
        BitWriter_write(writer, codes[j], num_bits[j]);
        break;
      }
    }
  }
  BitWriter_flush(writer);
  BitWriter_free(writer);

  Huffman_free(root);
  free(codes);
  free(num_bits);

  fclose(src);
  fclose(dst);
  return 0;
}
