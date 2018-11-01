/*
 *  Feng Yanlin   1600012760@pku.edu.cn
 *
 *  test.c - test compression algorithms
 *
 */

#include <stdio.h>
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
