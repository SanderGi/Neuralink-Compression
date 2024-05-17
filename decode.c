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
  parse_header(src, raw_header, &num_samples, &size_of_each_sample);

  // copy the raw header
  if (fwrite(raw_header, RAW_HEADER_SIZE, 1, dst) != 1) {
    printf("Error writing file\n");
    exit(1);
  }

  // read number of unique values
  int num_unique_values;
  if (fread(&num_unique_values, sizeof(int), 1, src) != 1) {
    printf("Error reading file\n");
    exit(1);
  }
  if (num_unique_values > 512) {
    execvp("./unique_2dot00/decode", argv);
  }

  // load the huffman tree
  struct HuffmanNode *root = Huffman_load(src);

  // read the data samples
  if (DEBUG) printf("Number of samples: %ld\n", num_samples);
  short datas[num_samples];
  struct BitReader *reader = BitReader_alloc(src);
  for (long i = 0; i < num_samples; i++) {
    struct HuffmanNode *node = root;
    while (node->one != NULL) {
      if (BitReader_read(reader, 1)) {
        node = node->one;
      } else {
        node = node->zero;
      }
    }
    datas[i] = node->value;
  }
  BitReader_free(reader);

  Huffman_free(root);

  // write the data samples
  if (fwrite(datas, sizeof(short) * num_samples, 1, dst) != 1) {
    printf("Error writing file\n");
    exit(1);
  }

  fclose(src);
  fclose(dst);
  return 0;
}
