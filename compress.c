/*
 *  Feng Yanlin   1600012760@pku.edu.cn
 *
 *  compress.c - encoder and decoder for Gamma-Code, Variable-Byte and Delta-Code
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "compress.h"


/*------------------------------------------------------------------------
 *  print_bits  -  Print bitstream (for debug)
 *------------------------------------------------------------------------
 */
void print_bits(struct bitstream_t bitstream){
    int i;
    char * p;
    unsigned char pmask = 0x80;

    p = bitstream.buf;
    for (i = 0; i < bitstream.nbits; i++){
        putchar("10"[!(*p & pmask)]);
        if ((pmask = pmask >> 1) == 0)  { p++; pmask = 0x80; }
    }
    putchar('\n');
}


/*------------------------------------------------------------------------
 *  print_array  -  Print array (for debug)
 *------------------------------------------------------------------------
 */
void print_array(struct array_t nums){
    int i;
    for (i = 0; i < nums.nsize; i++)
        printf("%u ", nums.buf[i]);
    printf("\n");
}


/*------------------------------------------------------------------------
 *  gamma_encode  -  Encode Gamma integers
 *------------------------------------------------------------------------
 */
struct bitstream_t gamma_encode(struct array_t nums){
    struct bitstream_t bs = {NULL, 0}, bs2;
    uint32_t i, j, *q, l, n = 0, nbytes, p = 0, lb;
    unsigned char t[8], tmp;
    for (i = 0, q = nums.buf; i < nums.nsize; i++, q++){  // first pass: determine buffer size
        for (l = 0, n = *q; n = n >> 1;)  l++;
        bs.nbits += (l << 1) + 1;
    }
    bs.buf = calloc( (((bs.nbits + 7) >> 3) + 15) & ~7, 1);  // double-word aligned
    for (i = 0, q = nums.buf; i < nums.nsize; i++, q++){
        for (l = 0, n = *q; n = n >> 1;)  l++;
        lb = ((p & 7) + l + 7) >> 3;
        *(uint64_t *)t = (uint64_t)((1 << l) - 1) << ((lb << 3) - (p & 7) - l);
        for (j = 0; lb && j < lb-j-1; j++)  { tmp = t[j]; t[j] = t[lb-j-1]; t[lb-j-1] = tmp; }
        *(uint64_t *)(bs.buf + (p >> 3)) |= *(uint64_t *)t;  // write length
        p += l + 1;
        lb = ((p & 7) + l + 7) >> 3;
        *(uint64_t *)t = (uint64_t)(*q & ((1 << l) - 1)) << ((lb << 3) - (p & 7) - l);
        for (j = 0; lb && j < lb-j-1; j++)  { tmp = t[j]; t[j] = t[lb-j-1]; t[lb-j-1] = tmp; }
        *(uint64_t *)(bs.buf + (p >> 3)) |= *(uint64_t *)t;  // write offset
        p += l;
    }
    return bs;
}


/*------------------------------------------------------------------------
 *  gamma_encode_bitwise  -  Bitwise version of gamma_encode (slower)
 *------------------------------------------------------------------------
 */

struct bitstream_t gamma_encode_bitwise(struct array_t nums){
    uint32_t n, l, i;
    struct bitstream_t bs = { NULL, 0 };
    char * p;
    unsigned char pm = 0x80;

    for (i = 0; i < nums.nsize; i++){
        for (n = nums.buf[i], l = 0; (n = n >> 1) > 0;)  l++;
        bs.nbits += (l << 1) + 1;
    }
    bs.buf = calloc((bs.nbits + 7) / 8, 1);
    for (i = 0, p = bs.buf; i < nums.nsize; i++){
        for (n = nums.buf[i], l = 0; (n = n >> 1) > 0; l++){
            *p |= pm;
            if ((pm = pm >> 1) == 0)  { p++; pm = 0x80; }
        }
        if ((pm = pm >> 1) == 0)  { p++; pm = 0x80; }
        n = nums.buf[i] << (32 - l);
        while (l-- > 0){
            *p |= pm & (unsigned char)-((n & 0x80000000) >> 31);
            n = n << 1;
            if ((pm = pm >> 1) == 0)  { p++; pm = 0x80; }
        }
    }
    return bs;
}


/*------------------------------------------------------------------------
 *  gamma_decode  -  Decode Gamma integers
 *------------------------------------------------------------------------
 */
struct array_t gamma_decode(struct bitstream_t bs){
    char * p;
    unsigned char pm = 0x80;
    uint32_t n, l, i, j, nbits = 0, *q;
    struct array_t nums = {NULL, 0};

    for (p = bs.buf; nbits < bs.nbits; nums.nsize++){
        for (l = 0; *p & pm; l++, nbits++)  if ((pm = pm >> 1) == 0)  { p++; pm = 0x80; }
        nbits += 1 + l;
        p = bs.buf + (nbits >> 3);
        pm = 0x80 >> (nbits & 7);
    }
    nums.buf = malloc(nums.nsize << 5);
    for (i = 0, q = nums.buf, p = bs.buf, pm = 0x80; i < nums.nsize; i++){
        for (l = 0; *p & pm; l++)  if ((pm = pm >> 1) == 0)  { p++; pm = 0x80; }
        if ((pm = pm >> 1) == 0)  { p++; pm = 0x80; }
        for (n = 1, j = 0; j < l; j++){
            n = (n << 1) + !!(*p & pm);
            if ((pm = pm >> 1) == 0)  { p++; pm = 0x80; }
        }
        *q++ = n;
    }
    return nums;
}


/*------------------------------------------------------------------------
 *  variable_byte_encode  -  Encode Variable-Byte integers
 *------------------------------------------------------------------------
 */

struct bitstream_t variable_byte_encode(struct array_t nums){
    uint32_t n, i, nbytes = 0, l, *q;
    char tmp[5], *p;
    struct bitstream_t bs;

    for (i = 0, q = nums.buf; i < nums.nsize; i++, q++){
        n = *q;
        while (n) { n /= 128; nbytes++; }
    }
    p = bs.buf = malloc(nbytes);
    bs.nbits = nbytes << 3;
    for (i = 0, q = nums.buf; i < nums.nsize; i++, q++){
        n = *q;
        l = 0;
        while(n)  { tmp[l++] = n % 128; n /= 128; }
        while (l--)  *p++ = tmp[l];
        *(p - 1) += 128;
    }
    return bs;
}


/*------------------------------------------------------------------------
 *  variable_byte_decode  -  Decode Variable-Byte integers
 *------------------------------------------------------------------------
 */
struct array_t variable_byte_decode(struct bitstream_t bs){
    uint32_t i, nbytes, nsize = 0, *p, n = 0;
    struct array_t nums;
    char *q;

    nbytes = bs.nbits >> 3;
    for (i = 0, q = bs.buf; i < nbytes; i++, q++)
        if (*q & 0x80)  nsize++;
    p = (uint32_t *)(nums.buf = malloc(nsize << 2));
    nums.nsize = nsize;
    for (i = 0, q = bs.buf; i < nbytes; i++, q++){
        n = (n << 7) + (*q & 0x7f);
        if (*q & 0x80)  { *p++ = n; n = 0; }
    }
    return nums;
}


/*------------------------------------------------------------------------
 *  delta_encode  -  Encode Delta integers
 *------------------------------------------------------------------------
 */
struct bitstream_t delta_encode(struct array_t nums){
    struct bitstream_t bs = {NULL, 0};
    uint32_t i, j, *q, l, ll, lt, n = 0, nbytes, p = 0, lb;
    unsigned char t[8], tmp;
    for (i = 0, q = nums.buf; i < nums.nsize; i++, q++){  // first pass: determine buffer size
        for (l = 0, n = *q; n = n >> 1;)  l++;
        bs.nbits += l;
        for (ll = 0; l = l >> 1;)  ll++;
        bs.nbits += (ll << 1) + 1;
    }
    bs.buf = calloc( (((bs.nbits + 7) >> 3) + 15) & ~7, 1);  // double-word aligned
    for (i = 0, q = nums.buf; i < nums.nsize; i++, q++){
        for (l = 0, n = *q; n = n >> 1;)  l++;
        for (ll = 0, lt = l; lt = lt >> 1;)  ll++;
        lb = ((p & 7) + ll + 7) >> 3;
        *(uint64_t *)t = (uint64_t)((1 << ll) - 1) << ((lb << 3) - (p & 7) - ll);
        for (j = 0; lb && j < lb-j-1; j++)  { tmp = t[j]; t[j] = t[lb-j-1]; t[lb-j-1] = tmp; }
        *(uint64_t *)(bs.buf + (p >> 3)) |= *(uint64_t *)t;  // write l-length
        p += ll + 1;
        lb = ((p & 7) + ll + 7) >> 3;
        *(uint64_t *)t = (uint64_t)(l & ((1 << ll) - 1)) << ((lb << 3) - (p & 7) - ll);
        for (j = 0; lb && j < lb-j-1; j++)  { tmp = t[j]; t[j] = t[lb-j-1]; t[lb-j-1] = tmp; }
        *(uint64_t *)(bs.buf + (p >> 3)) |= *(uint64_t *)t;  // write l-offset
        p += ll;
        lb = ((p & 7) + l + 7) >> 3;
        *(uint64_t *)t = (uint64_t)(*q & ((1 << l) - 1)) << ((lb << 3) - (p & 7) - l);
        for (j = 0; lb && j < lb-j-1; j++)  { tmp = t[j]; t[j] = t[lb-j-1]; t[lb-j-1] = tmp; }
        *(uint64_t *)(bs.buf + (p >> 3)) |= *(uint64_t *)t;  // write offset
        p += l;
    }
    return bs;
}


/*------------------------------------------------------------------------
 *  delta_decode  -  Decode Delta integers
 *------------------------------------------------------------------------
 */
struct array_t delta_decode(struct bitstream_t bs){
    char * p;
    unsigned char pm = 0x80;
    uint32_t n, l, ll, i, j, nbits = 0, *q;
    struct array_t nums = {NULL, 0};

    for (p = bs.buf; nbits < bs.nbits; nums.nsize++){
        for (ll = 0; *p & pm; ll++)  if ((pm = pm >> 1) == 0)  { p++; pm = 0x80; }
        if ((pm = pm >> 1) == 0)  { p++; pm = 0x80; }
        nbits += (ll << 1) + 1;
        for (l = 1; ll > 0; ll--){
            l = (l << 1) + !!(*p & pm);
            if ((pm = pm >> 1) == 0)  { p++; pm = 0x80; }
        }
        nbits += l;
        p = bs.buf + (nbits >> 3);
        pm = 0x80 >> (nbits & 7);
    }
    nums.buf = malloc(nums.nsize << 5);
    for (i = 0, q = nums.buf, p = bs.buf, pm = 0x80; i < nums.nsize; i++){
        for (ll = 0; *p & pm; ll++)  if ((pm = pm >> 1) == 0)  { p++; pm = 0x80; }
        if ((pm = pm >> 1) == 0)  { p++; pm = 0x80; }
        for (l = 1; ll > 0; ll--){
            l = (l << 1) + !!(*p & pm);
            if ((pm = pm >> 1) == 0)  { p++; pm = 0x80; }
        }
        for (n = 1; l > 0; l--){
            n = (n << 1) + !!(*p & pm);
            if ((pm = pm >> 1) == 0)  { p++; pm = 0x80; }
        }
        *q++ = n;
    }
    return nums;
}



int main(){
    struct bitstream_t bitstream;

    uint32_t nums[] = {2, 3, 4, 5, 13, 
                       128, 91, 4, 2, 80,
                       1024, 24924, 0x80000000, 0x7fffffff, 0xffffffff};
    struct array_t array = {nums, 15};

    printf("Original integers:  ");
    print_array(array);
    printf("\n");
    printf("Testing Gamma-Code...\n");
    printf("Encoded Gamma integers:  ");
    print_bits(gamma_encode(array));
    printf("Decoded Gamma integers:  ");    
    print_array(gamma_decode(gamma_encode(array)));
    printf("\n");
    printf("Testing Variable-Byte...\n");
    printf("Encoded Variable-Byte integers:  ");
    print_bits(variable_byte_encode(array));
    printf("Decoded Variable-Byte integers:  ");    
    print_array(variable_byte_decode(variable_byte_encode(array)));
    printf("\n");
    printf("Testing Delta-Code...\n");
    printf("Encoded Delta integers:  ");
    print_bits(delta_encode(array));
    printf("Decoded Delta integers:  ");    
    print_array(delta_decode(delta_encode(array)));
    printf("\n");

    return 0;
}