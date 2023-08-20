// Header for pixFilt routines
// (c)2013 Noudio
#ifndef __PIXFILT_H__
#define __PIXFILT_H__
// ---------------------------------------------------------
#include <stdint.h>

// like to hide mulAcc details from user, but not std pixFilt fields
#include "mulacc.h"

typedef struct pixFilt_s {
    int        tapsNr;
    int        polyNr;
    int8_t     *taps;    //taps values: 0 -> 1/256 // 127 -> 255/256 // -128 -> -255/256
    mulAccIn_t *fifo;
    mulAccIn_t *fifoPtr;
} pixFilt_t;

// ----------------------------------------------------------
pixFilt_t  *createPixFiltNearestNeighbor (void);
pixFilt_t  *createPixFiltBiLinear (void);
void       destroyPixFilt         (pixFilt_t *R);
void       pixFiltIn              (pixFilt_t *F, uint32_t P);
uint32_t   pixFiltOut             (pixFilt_t *F, int Phase);

#ifdef   __PIXFILT_TEST__
void      test_pixfilt            (void);
#endif //__PIXFILT_TEST__

#endif // __PIXFILT_H__
