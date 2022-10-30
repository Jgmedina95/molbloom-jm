#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "0.1.0"
#define FILE_VERSION 1u
#define min(X, Y) (((X) < (Y)) ? (X) : (Y))
#define max(X, Y) (((X) > (Y)) ? (X) : (Y))

uint64_t FNV1a(const char *str)
{
  uint64_t hash = 14695981039346656037ULL;
  for (int i = 0; str[i]; i++)
  {
    hash ^= str[i];
    hash *= 1099511628211ULL;
  }
  return hash;
}

// just changed constants
uint64_t FNV1ab(const char *str)
{
  uint64_t hash = 6281386704453804504ULL;
  for (int i = 0; str[i]; i++)
  {
    hash ^= str[i];
    hash *= 245453790232ULL;
  }
  return hash;
}

// bloom filter data
typedef struct
{
  uint32_t k;    // number of hash functions
  uint64_t m;    // size of bit array
  char name[32]; // name of filter
  char *data;
} bloom_t;

// create a new bloom filter
bloom_t *bloom_new(uint64_t size, uint64_t n, const char *name)
{

  bloom_t *b = malloc(sizeof(bloom_t));
  b->m = size;
  // 8 bits per byte
  b->data = calloc(size / 8, 1);
  if (strlen(name) > 32)
  {
    fprintf(stderr, "bloom_new: name too long\n");
    return NULL;
  }
  else
  {
    strcpy(b->name, name);
  }

  // compute number of hash functions
  b->k = max(8, min(64, (char)(b->m / n * log(2))));

  // print info and false positive rate
  printf("bloom_new: %s size=%lu bits, MB=%2f, n=%lu k=%u fp=%f\n", b->name, b->m,
         ((float)b->m) / 1024 / 1024 / 8, n, b->k,
         pow(1 - exp(-(float)(b->k * n) / (b->m)), b->k));

  return b;
}

// free a bloom filter
void bloom_free(bloom_t *b)
{
  free(b->data);
  free(b);
}

// add a string to a bloom filter
void bloom_add(bloom_t *b, char *s)
{
  uint64_t h;
  uint64_t h1 = FNV1a(s);
  uint64_t h2 = FNV1ab(s);
  for (unsigned int i = 0; i < b->k; i++)
  {
    // Building a Better Bloom Filter
    h = (h1 + i * h2) % b->m;
    b->data[h / 8] |= 1 << (h % 8);
  }
}

// check if a string is in a bloom filter
int bloom_check(bloom_t *b, char *s)
{
  uint64_t h;
  uint64_t h1 = FNV1a(s);
  uint64_t h2 = FNV1ab(s);
  for (unsigned int i = 0; i < b->k; i++)
  {
    // Building a Better Bloom Filter
    h = (h1 + i * h2) % b->m;
    if (!(b->data[h / 8] & (1 << (h % 8))))
      return 0;
  }
  return 1;
}

// write file
void bloom_write(bloom_t *b, char *filename)
{
  FILE *f = fopen(filename, "wb");
  // write magic number
  fwrite("BLOOM", 1, 5, f);
  // write version
  putc(FILE_VERSION, f);
  // write k and m
  fwrite(&b->k, sizeof(uint32_t), 1, f);
  fwrite(&b->m, sizeof(uint64_t), 1, f);
  fwrite(b->data, 1, b->m / 8, f);
  fclose(f);
}

bloom_t *bloom_read(char *filename)
{
  FILE *f = fopen(filename, "rb");
  char magic[5];
  fread(magic, 1, 5, f);
  if (strcmp(magic, "BLOOM") != 0)
  {
    fprintf(stderr, "bloom_read: invalid magic number\n");
    return NULL;
  }
  uint8_t version = getc(f);
  if (version != FILE_VERSION)
  {
    fprintf(stderr, "bloom_read: invalid version number\n");
    return NULL;
  }
  uint32_t k;
  uint64_t m;
  fread(&k, sizeof(uint32_t), 1, f);
  fread(&m, sizeof(uint64_t), 1, f);
  bloom_t *b = malloc(sizeof(bloom_t));
  b->k = k;
  b->m = m;
  b->data = malloc(m / 8);
  strcpy(b->name, "loaded bloom filter");
  fread(b->data, 1, m / 8, f);
  fclose(f);

  printf("bloom_read: %s size=%lu bits, MB=%2f, k=%u\n", b->name, b->m,
         ((float)b->m) / 1024 / 1024 / 8, b->k);
  return b;
}
