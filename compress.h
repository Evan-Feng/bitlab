/*
 *  Feng Yanlin   1600012760@pku.edu.cn
 *
 *  compress.h - struct bitstream_t, struct array_t
 *
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


struct bitstream_t {
    char *buf;
    int nbits;
};

struct array_t {
    uint32_t *buf;
    int nsize;
};


extern struct bitstream_t gamma_encode(struct array_t);

extern struct array_t gamma_decode(struct bitstream_t);

extern struct bitstream_t variable_byte_encode(struct array_t);

extern struct array_t variable_byte_decode(struct bitstream_t);

extern struct bitstream_t delta_encode(struct array_t);

extern struct array_t delta_decode(struct bitstream_t);
