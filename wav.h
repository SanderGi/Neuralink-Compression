#ifndef WAV_H
#define WAV_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0
#define DEBUG FALSE

#define RAW_HEADER_SIZE 44

struct wav_header {
  unsigned char riff[5];              // RIFF string
  unsigned int overall_size;          // overall size of file in bytes
  unsigned char wave[5];              // WAVE string
  unsigned char fmt_chunk_marker[4];  // fmt string with trailing null char
  unsigned int length_of_fmt;         // length of the format data
  unsigned int format_type;  // format type. 1-PCM, 3- IEEE float, 6 - 8bit A
                             // law, 7 - 8bit mu law
  unsigned int channels;     // no.of channels
  unsigned int sample_rate;  // sampling rate (blocks per second)
  unsigned int byterate;     // SampleRate * NumChannels * BitsPerSample/8
  unsigned int block_align;  // NumChannels * BitsPerSample/8
  unsigned int bits_per_sample;  // bits per sample, 8- 8bits, 16- 16 bits etc
  unsigned char data_chunk_header[5];  // DATA string or FLLR string
  unsigned int data_size;  // NumSamples * NumChannels * BitsPerSample/8 - size
                           // of the next chunk that will be read
};

int little_to_big_endian(unsigned char buffer[4]);

struct wav_header parse_header(FILE* src,
                               unsigned char raw_header[RAW_HEADER_SIZE],
                               long* num_samples, long* size_of_each_sample);

long read_data_sample(unsigned char* data_buffer, char data_buffer_size);

struct BitReader {
  FILE* fd;
  unsigned char buffer;
  char buffer_size;
};

struct BitReader* BitReader_alloc(FILE* reader);
void BitReader_free(struct BitReader* reader);
long BitReader_read(struct BitReader* reader, int num_bits);

struct BitWriter {
  FILE* fd;
  unsigned char buffer;
  char buffer_size;
};

struct BitWriter* BitWriter_alloc(FILE* writer);
void BitWriter_free(struct BitWriter* writer);
void BitWriter_write(struct BitWriter* writer, long data, int num_bits);
void BitWriter_flush(struct BitWriter* writer);

struct CapacityQueue {
  int capacity;
  int size;
  int front;
  void** data;
};

struct CapacityQueue* CapacityQueue_alloc(int capacity);
void CapacityQueue_free(struct CapacityQueue* queue);
void CapacityQueue_push(struct CapacityQueue* queue, void* value);
void* CapacityQueue_pop(struct CapacityQueue* queue);
void* CapacityQueue_peek(struct CapacityQueue* queue);

struct HuffmanNode {
  short value;
  short frequency;
  struct HuffmanNode* zero;
  struct HuffmanNode* one;
};

// unique_values and freq_counts must be sorted by frequency ascending
struct HuffmanNode* Huffman_root(short* unique_values, short* freq_counts,
                                 int num_unique_values);
void Huffman_free(struct HuffmanNode* root);
// gets the Huffman codes for each unique value
long* Huffman_codes(struct HuffmanNode* root, short* unique_values,
                    int num_unique_values, char** num_bits);
void Huffman_save(FILE* dst, struct HuffmanNode* root);
struct HuffmanNode* Huffman_load(FILE* src);

#endif  // WAV_H
