#include "wav.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int little_to_big_endian(unsigned char buffer[4]) {
  return buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
}

long read_data_sample(unsigned char* data_buffer, char data_buffer_size) {
  long data;
  if (data_buffer_size == 4) {
    data = little_to_big_endian(data_buffer);
  } else if (data_buffer_size == 2) {
    data = (data_buffer[0] & 0x00ff) | (data_buffer[1] << 8);
  } else if (data_buffer_size == 1) {
    data = data_buffer[0] & 0x00ff;
    data -= 128;  // in wave, 8-bit are unsigned, so shifting to signed
  } else {
    printf("Unsupported data_buffer_size: %d\n", data_buffer_size);
    exit(1);
  }
  return data;
}

// https://truelogic.org/wordpress/2015/09/04/parsing-a-wav-file-in-c/
struct wav_header parse_header(FILE* src,
                               unsigned char raw_header[RAW_HEADER_SIZE],
                               long* num_samples, long* size_of_each_sample) {
  struct wav_header header;

  if (fread(raw_header, RAW_HEADER_SIZE, 1, src) != 1) {
    printf("Error reading file\n");
    exit(1);
  }

  header.riff[0] = raw_header[0];
  header.riff[1] = raw_header[1];
  header.riff[2] = raw_header[2];
  header.riff[3] = raw_header[3];
  header.riff[4] = '\0';
  if (DEBUG) printf("(1-4): %s\n", header.riff);

  header.overall_size = little_to_big_endian(&raw_header[4]);
  if (DEBUG)
    printf("%u %u %u %u\n", raw_header[4], raw_header[5], raw_header[6],
           raw_header[7]);
  if (DEBUG)
    printf("(5-8) Overall size: bytes:%u, Kb:%u\n", header.overall_size,
           header.overall_size / 1024);

  header.wave[0] = raw_header[8];
  header.wave[1] = raw_header[9];
  header.wave[2] = raw_header[10];
  header.wave[3] = raw_header[11];
  header.wave[4] = '\0';
  if (DEBUG) printf("(9-12) Wave marker: %s\n", header.wave);

  header.fmt_chunk_marker[0] = raw_header[12];
  header.fmt_chunk_marker[1] = raw_header[13];
  header.fmt_chunk_marker[2] = raw_header[14];
  header.fmt_chunk_marker[3] = '\0';
  if (DEBUG) printf("(13-16) Fmt marker: %s\n", header.fmt_chunk_marker);

  if (DEBUG)
    printf("%u %u %u %u\n", raw_header[16], raw_header[17], raw_header[18],
           raw_header[19]);
  header.length_of_fmt = little_to_big_endian(&raw_header[16]);
  if (DEBUG) printf("(17-20) Length of Fmt header: %u\n", header.length_of_fmt);

  if (DEBUG) printf("%u %u\n", raw_header[20], raw_header[21]);
  header.format_type = raw_header[20] | (raw_header[21] << 8);
  char format_name[10] = "";
  if (header.format_type == 1)
    strcpy(format_name, "PCM");
  else if (header.format_type == 6)
    strcpy(format_name, "A-law");
  else if (header.format_type == 7)
    strcpy(format_name, "Mu-law");
  if (DEBUG)
    printf("(21-22) Format type: %u %s\n", header.format_type, format_name);

  if (DEBUG) printf("%u %u\n", raw_header[22], raw_header[23]);
  header.channels = raw_header[22] | (raw_header[23] << 8);
  if (DEBUG) printf("(23-24) Channels: %u\n", header.channels);

  if (DEBUG)
    printf("%u %u %u %u\n", raw_header[24], raw_header[25], raw_header[26],
           raw_header[27]);
  header.sample_rate = little_to_big_endian(&raw_header[24]);
  if (DEBUG) printf("(25-28) Sample rate: %u\n", header.sample_rate);

  if (DEBUG)
    printf("%u %u %u %u\n", raw_header[28], raw_header[29], raw_header[30],
           raw_header[31]);
  header.byterate = little_to_big_endian(&raw_header[28]);
  if (DEBUG)
    printf("(29-32) Byte Rate: %u, Bit Rate:%u\n", header.byterate,
           header.byterate * 8);

  if (DEBUG) printf("%u %u\n", raw_header[32], raw_header[3]);
  header.block_align = raw_header[32] | (raw_header[33] << 8);
  if (DEBUG) printf("(33-34) Block Alignment: %u\n", header.block_align);

  if (DEBUG) printf("%u %u\n", raw_header[34], raw_header[35]);
  header.bits_per_sample = raw_header[34] | (raw_header[35] << 8);
  if (DEBUG) printf("(35-36) Bits per sample: %u\n", header.bits_per_sample);

  header.data_chunk_header[0] = raw_header[36];
  header.data_chunk_header[1] = raw_header[37];
  header.data_chunk_header[2] = raw_header[38];
  header.data_chunk_header[3] = raw_header[39];
  header.data_chunk_header[4] = '\0';
  if (DEBUG) printf("(37-40) Data Marker: %s\n", header.data_chunk_header);

  if (DEBUG)
    printf("%u %u %u %u\n", raw_header[40], raw_header[41], raw_header[42],
           raw_header[43]);
  header.data_size = little_to_big_endian(&raw_header[40]);
  if (DEBUG) printf("(41-44) Size of data chunk: %u\n", header.data_size);

  // calculate no. of samples and size of each sample
  *num_samples =
      (8 * header.data_size) / (header.channels * header.bits_per_sample);
  if (DEBUG) printf("Number of samples: %lu\n", *num_samples);

  *size_of_each_sample = (header.channels * header.bits_per_sample) / 8;
  if (DEBUG) printf("Size of each sample: %ld bytes\n", *size_of_each_sample);

  if (DEBUG) {
    // calculate duration of file
    float duration_in_seconds = (float)header.overall_size / header.byterate;
    printf("Approx. duration in seconds: %f\n", duration_in_seconds);

    // the valid amplitude range for values based on the bits per sample
    long low_limit = 0l;
    long high_limit = 0l;

    switch (header.bits_per_sample) {
      case 8:
        low_limit = -128;
        high_limit = 127;
        break;
      case 16:
        low_limit = -32768;
        high_limit = 32767;
        break;
      case 32:
        low_limit = -2147483648;
        high_limit = 2147483647;
        break;
    }
    printf("Valid range for data values: %ld to %ld\n", low_limit, high_limit);
  }

  return header;
}

struct BitReader* BitReader_alloc(FILE* reader) {
  struct BitReader* bit_reader = malloc(sizeof(struct BitReader));
  bit_reader->fd = reader;
  bit_reader->buffer_size = 0;
  return bit_reader;
}
void BitReader_free(struct BitReader* reader) { free(reader); }
long BitReader_read(struct BitReader* reader, int num_bits) {
  if (num_bits > 64) {
    printf("Cannot read more than 64 bits at a time\n");
    exit(1);
  }
  long result = 0;
  while (num_bits > 0) {
    if (reader->buffer_size == 0) {
      if (fread(&reader->buffer, 1, 1, reader->fd) != 1) {
        printf("Error reading file\n");
        exit(1);
      }
      reader->buffer_size = 8;
    }
    int bits_to_read =
        num_bits < reader->buffer_size ? num_bits : reader->buffer_size;
    int bits_to_shift = reader->buffer_size - bits_to_read;
    result <<= bits_to_read;
    result |= (reader->buffer >> bits_to_shift) & ((1 << bits_to_read) - 1);
    num_bits -= bits_to_read;
    reader->buffer_size -= bits_to_read;
  }
  return result;
}

struct BitWriter* BitWriter_alloc(FILE* writer) {
  struct BitWriter* bit_writer = malloc(sizeof(struct BitWriter));
  bit_writer->fd = writer;
  bit_writer->buffer_size = 0;
  return bit_writer;
}
void BitWriter_free(struct BitWriter* writer) { free(writer); }
// writes the num_bits least significant bits of data
void BitWriter_write(struct BitWriter* writer, long data, int num_bits) {
  if (num_bits > 128) {
    printf("Cannot write more than 128 bits at a time\n");
    exit(1);
  }
  while (num_bits > 0) {
    int bits_to_write = 8 - writer->buffer_size;
    if (bits_to_write > num_bits) {
      bits_to_write = num_bits;
    }
    writer->buffer <<= bits_to_write;
    writer->buffer |=
        (data >> (num_bits - bits_to_write)) & ((1 << bits_to_write) - 1);
    writer->buffer_size += bits_to_write;
    num_bits -= bits_to_write;
    if (writer->buffer_size == 8) {
      if (fwrite(&writer->buffer, 1, 1, writer->fd) != 1) {
        printf("Error writing file\n");
        exit(1);
      }
      writer->buffer_size = 0;
    }
  }
}
void BitWriter_flush(struct BitWriter* writer) {
  writer->buffer <<= 8 - writer->buffer_size;
  if (writer->buffer_size > 0) {
    if (fwrite(&writer->buffer, 1, 1, writer->fd) != 1) {
      printf("Error writing file\n");
      exit(1);
    }
    writer->buffer_size = 0;
  }
}

struct CapacityQueue* CapacityQueue_alloc(int capacity) {
  struct CapacityQueue* queue = malloc(sizeof(struct CapacityQueue));
  queue->capacity = capacity;
  queue->size = 0;
  queue->front = 0;
  queue->data = malloc(sizeof(void*) * capacity);
  return queue;
}
void CapacityQueue_free(struct CapacityQueue* queue) {
  free(queue->data);
  free(queue);
}
void CapacityQueue_push(struct CapacityQueue* queue, void* value) {
  if (queue->size == queue->capacity) {
    printf("Queue is full\n");
    exit(1);
  }
  queue->data[(queue->front + queue->size) % queue->capacity] = value;
  queue->size++;
}
void* CapacityQueue_pop(struct CapacityQueue* queue) {
  if (queue->size == 0) {
    return NULL;
  }
  void* value = queue->data[queue->front];
  queue->front = (queue->front + 1) % queue->capacity;
  queue->size--;
  return value;
}
void* CapacityQueue_peek(struct CapacityQueue* queue) {
  if (queue->size == 0) {
    return NULL;
  }
  return queue->data[queue->front];
}

struct HuffmanNode* Huffman_root(short* unique_values, short* freq_counts,
                                 int num_unique_values) {
  struct CapacityQueue* queue1 = CapacityQueue_alloc(num_unique_values);
  struct CapacityQueue* queue2 = CapacityQueue_alloc(num_unique_values);

  for (int i = 0; i < num_unique_values; i++) {
    struct HuffmanNode* node = malloc(sizeof(struct HuffmanNode));
    node->value = unique_values[i];
    node->frequency = freq_counts[i];
    node->zero = NULL;
    node->one = NULL;
    CapacityQueue_push(queue1, node);
  }

  while (queue1->size > 0 || queue2->size > 0) {
    struct HuffmanNode* node1;
    struct HuffmanNode* node2;

    if (CapacityQueue_peek(queue1) == NULL) {
      if (queue2->size == 1) {
        return CapacityQueue_pop(queue2);
      } else {
        node1 = CapacityQueue_pop(queue2);
        node2 = CapacityQueue_pop(queue2);
      }
    } else if (CapacityQueue_peek(queue2) == NULL) {
      if (queue1->size == 1) {
        return CapacityQueue_pop(queue1);
      } else {
        node1 = CapacityQueue_pop(queue1);
        node2 = CapacityQueue_pop(queue1);
      }
    } else {
      if (((struct HuffmanNode*)CapacityQueue_peek(queue1))->frequency <
          ((struct HuffmanNode*)CapacityQueue_peek(queue2))->frequency) {
        node1 = CapacityQueue_pop(queue1);
      } else {
        node1 = CapacityQueue_pop(queue2);
      }

      if (CapacityQueue_peek(queue1) == NULL) {
        node2 = CapacityQueue_pop(queue2);
      } else if (CapacityQueue_peek(queue2) == NULL) {
        node2 = CapacityQueue_pop(queue1);
      } else {
        if (((struct HuffmanNode*)CapacityQueue_peek(queue1))->frequency <
            ((struct HuffmanNode*)CapacityQueue_peek(queue2))->frequency) {
          node2 = CapacityQueue_pop(queue1);
        } else {
          node2 = CapacityQueue_pop(queue2);
        }
      }
    }

    struct HuffmanNode* internal_node = malloc(sizeof(struct HuffmanNode));
    internal_node->frequency = node1->frequency + node2->frequency;
    internal_node->zero = node1;
    internal_node->one = node2;
    CapacityQueue_push(queue2, internal_node);
  }

  return NULL;  // unreachable
}
void Huffman_free(struct HuffmanNode* root) {
  if (root->zero != NULL) {
    Huffman_free(root->zero);
  }
  if (root->one != NULL) {
    Huffman_free(root->one);
  }
  free(root);
}
static int is_leaf(struct HuffmanNode* node) {
  return node->zero == NULL && node->one == NULL;
}
static void Huffman_codesHelper(struct HuffmanNode* node, int code, char bits,
                                long* codes, char* num_bits,
                                short* unique_values, int num_unique_values) {
  if (is_leaf(node)) {
    for (int i = 0; i < num_unique_values; i++) {
      if (node->value == unique_values[i]) {
        codes[i] = code;
        num_bits[i] = bits;
        return;
      }
    }
  } else {
    Huffman_codesHelper(node->zero, code << 1, bits + 1, codes, num_bits,
                        unique_values, num_unique_values);
    Huffman_codesHelper(node->one, (code << 1) | 1, bits + 1, codes, num_bits,
                        unique_values, num_unique_values);
  }
}
long* Huffman_codes(struct HuffmanNode* root, short* unique_values,
                    int num_unique_values, char** num_bits) {
  long* codes = malloc(sizeof(long) * num_unique_values);
  *num_bits = malloc(sizeof(char) * num_unique_values);

  Huffman_codesHelper(root, 0, 0, codes, *num_bits, unique_values,
                      num_unique_values);
  return codes;
}
static void Huffman_SaveHelper(struct BitWriter* writer,
                               struct HuffmanNode* node) {
  if (is_leaf(node)) {
    BitWriter_write(writer, 1, 1);
    BitWriter_write(writer, node->value, 16);
  } else {
    BitWriter_write(writer, 0, 1);
    Huffman_SaveHelper(writer, node->zero);
    Huffman_SaveHelper(writer, node->one);
  }
}
void Huffman_save(FILE* dst, struct HuffmanNode* root) {
  struct BitWriter* writer = BitWriter_alloc(dst);
  Huffman_SaveHelper(writer, root);
  BitWriter_flush(writer);
  BitWriter_free(writer);
}
static struct HuffmanNode* Huffman_LoadHelper(struct BitReader* reader) {
  struct HuffmanNode* node = malloc(sizeof(struct HuffmanNode));
  if (BitReader_read(reader, 1) == 1) {
    node->value = BitReader_read(reader, 16);
    node->zero = NULL;
    node->one = NULL;
  } else {
    node->zero = Huffman_LoadHelper(reader);
    node->one = Huffman_LoadHelper(reader);
  }
  return node;
}
struct HuffmanNode* Huffman_load(FILE* src) {
  struct BitReader* reader = BitReader_alloc(src);
  struct HuffmanNode* root = Huffman_LoadHelper(reader);
  BitReader_free(reader);
  return root;
}
