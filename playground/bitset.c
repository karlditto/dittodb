#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CHAR_BITS (8 * sizeof(unsigned char))
#define WORD_BITS (8 * sizeof(unsigned int))

typedef struct {
  unsigned int size;
  unsigned int wordsize;
  unsigned int a[]; // flexible last array member is a c standard
} BitSet;

BitSet *bitset_init(int size) { bitset = malloc(sizeof(BitSet) +) }

int main() {
  unsigned int i = 0;
  unsigned int bits = 32 << (!i >> 63);
  printf("%zu\n", bits);
}
