/**************************
  pixel filt.c
***************************/
// (c)2013 Noudio
#ifndef __PIXFILT_C__
#define __PIXFILT_C__
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "pixfilt.h"

typedef struct {
   int        tapsNr;
   int        polyNr;
   uint8_t    padding[12];
} pixFiltPublic_t;

// -----------------------------------------------------
// protos
pixFilt_t  *createPixFilt         (int tapsNr,   int polyNr);
pixFilt_t  *createPixFiltBiLinear (void);
void       destroyPixFilt         (pixFilt_t *R);
void       pixFiltIn              (pixFilt_t *F, uint32_t P);
uint32_t   pixFiltOut             (pixFilt_t *F, int Phase);

// -----------------------------------------------------
void       destroyPixFilt (pixFilt_t *R)
{
   if (!R) {
      return;
   }
   if (R->taps) {
      free(R->taps);
      R->taps = NULL;
   }
   if (R->fifo) {
      free(R->fifo);
      R->fifo = NULL;
   }
   memset(R, 0, sizeof(pixFilt_t));
}
// -----------------------------------------------------
pixFilt_t *createPixFilt (int tapsNr,int polyNr)
{
   pixFilt_t *R;

   R = (pixFilt_t *)0;

   if ((tapsNr*polyNr <= 0) || (tapsNr>255) || (polyNr>255)) {
      destroyPixFilt(R);
      return NULL;
   }

   R = (pixFilt_t *)malloc(sizeof(pixFilt_t));
   if (!R) {
      destroyPixFilt(R);
      return NULL;
   }
   memset(R, 0, sizeof(pixFilt_t));

   R->taps = (int8_t *)malloc(tapsNr*polyNr*sizeof(int8_t));
   if (!R->taps) {
      destroyPixFilt(R);
      return NULL;
   }
   memset(R->taps, 0, tapsNr*polyNr*sizeof(int8_t));

   R->fifo = (mulAccIn_t *)malloc(tapsNr*sizeof(mulAccIn_t));
   if (!R->fifo) {
      destroyPixFilt(R);
      return NULL;
   }
   memset(R->fifo, 0, tapsNr*sizeof(mulAccIn_t));

   R->tapsNr  = tapsNr;
   R->polyNr  = polyNr;
   R->fifoPtr = R->fifo;
   return R;
}
// -----------------------------------------------------
int pixFiltTapsRearrange (pixFilt_t *F)
{
    // transpose taps Taps[tapIdx][polyIdx] ==> Taps[polyIdx][tapIdx]
    // This makes the taps per polyphase accessible with increases of one for a single poly:
    // --> thus it increases the prob, that the taps are all cached in one access

   int8_t *H; // workArea
   int8_t  *Tdst, *Tsrc;
   int     i,j, l;

   l = F->tapsNr*F->polyNr*sizeof(int8_t);
   H = (int8_t *)malloc(l);
   if (!H) {
      free(H);
      return -1;
   }
   // copy taps
   memcpy(H, F->taps, l);

   for(i=0; i<F->polyNr; i++) {

       Tdst = F->taps + i * F->tapsNr;   // Poly[i]:Tap[0]
       Tsrc = H + i;                     // Tap[0]:Poly[i]

       for (j=F->tapsNr; j; j--) {

           *Tdst = *Tsrc;
           Tdst++;                       // Poly[i]:Tap[j]
           Tsrc += F->polyNr;            // Tap[j]:Poly[i]

       }
   }
   free(H);
   return 0;
}

// -----------------------------------------------------
pixFilt_t *createPixFiltBiLinear (void)
{
   pixFilt_t *F;
   int       i;

   F=createPixFilt(2, 128);
   if (!F) {
      return F;
   }

   // tap0: past, tap1: recent, fracs running past linear to recent
   // polyPhases for tap[0]
   for (i=0; i<128; i++) {
      F->taps[i] = (int8_t)(127-i);
   }
   // polyPhases for tap[1]
   for (; i<256; i++) {
      F->taps[i] = (int8_t)(i-128);
   }

   if (pixFiltTapsRearrange (F) < 0) {
      destroyPixFilt (F);
      return (pixFilt_t *)0;
   }

   return F;
}
// -----------------------------------------------------
pixFilt_t *createPixFiltNearestNeighbor (void)
{
   pixFilt_t *F;
   int       i;

   F=createPixFilt(2, 128);
   if (!F) {
      return F;
   }

   // tap0: past, tap1: recent, fracs running past linear to recent
   // polyPhases for tap[0]
   for (i=0; i<128; i++) {
      F->taps[i] = (i<64) ? 127 : 0;
   }
   // polyPhases for tap[1]
   for (; i<256; i++) {
      F->taps[i] = (i<128+64) ? 0 : 127;
   }

   if (pixFiltTapsRearrange (F) < 0) {
      destroyPixFilt (F);
      return (pixFilt_t *)0;
   }
   return F;
}
// -----------------------------------------------------
void pixFiltIn(pixFilt_t *F, uint32_t P)
{
   // pre: F valid
   mulAccIn_t *S, *B, *E;

   S = F->fifoPtr;
   B = F->fifo;
   E = B + F->tapsNr;

   *S++ = mulAccInput(P);

   if (S >= E)
      S = B;

   F->fifoPtr = S;
}
// -----------------------------------------------------
uint32_t pixFiltOut(pixFilt_t *F, int Phase)
{
   // pre: F valid
   // pre: FiltIn was done First
   // pre: Phase from [0...F->polyNr--1]

   int8_t       *T;
   mulAccIn_t   *S, *B, *E;
   mulAccOut_t  A;

   int    i;

   mulAccInit(&A);

   T = F->taps + Phase*F->tapsNr;
   S = F->fifoPtr;
   B = F->fifo;
   E = B + F->tapsNr;

   for (i = F->tapsNr; i; i--) {
      mulAcc(&A, *T++, *S++);
      if (S >= E)
         S = B;
   }

   return mulAccExit(&A);
}
// -----------------------------------------------------
#ifdef __PIXFILT_TEST__
#include <stdio.h>

void test_pixfilt (void)
{
   pix32_t A, B, C;
   pixFilt_t *F;
   int        i;

   printf("sizeof rgb32_t:%d, pix32_t:%d acc32_t:%d pixFilt_t:%d pixFiltPublic_t:%d\n",
          sizeof(rgb32_t), sizeof(pix32_t), sizeof(acc32_t), sizeof(pixFilt_t),sizeof(pixFiltPublic_t));

   if (sizeof(pixFilt_t)!=sizeof(pixFiltPublic_t)) {
      printf("pixFilt_t: public size not right\n");
   }

   A.clr.A = 1;
   A.clr.R = 2;
   A.clr.G = 3;
   A.clr.B = 4;

   printf("A:0x%08x\n", A.pix);
   B.pix = ((uint32_t)A.clr.A << 24) |
           ((uint32_t)A.clr.R << 16) |
           ((uint32_t)A.clr.G <<  8) |
           ((uint32_t)A.clr.B <<  0);

   printf("B:0x%08x\n", B.pix);

  F = createPixFilt(8,32);
  printf("PixFilt: %p\n", F);
  if (F) {
     printf("  tapsNr:%d, polyNr:%d, taps:%p, fifo:%p, fifoPtr:%p\n",
            (int)F->tapsNr, (int)F->polyNr, F->taps, F->fifo, F->fifoPtr);
  }
  destroyPixFilt(F);

  F = createPixFiltBiLinear();
  printf("PixFilt: %p\n", F);
  if (F) {
     printf("  tapsNr:%d, polyNr:%d, taps:%p, fifo:%p, fifoPtr:%p\n",
            (int)F->tapsNr, (int)F->polyNr, F->taps, F->fifo, F->fifoPtr);
  }

   B.clr.A = 80;
   B.clr.R = 60;
   B.clr.G = 40;
   B.clr.B = 20;

   pixFiltIn(F, B.pix);
   pixFiltIn(F, A.pix);

  for (i=127; i<256; i++) {
     F->taps[i] = -F->taps[i];
  }

  for (i=0; i<128; i+=1) {
     C.pix = pixFiltOut(F, i);
     printf("i:%3d a:%3d r:%3d g:%3d b:%3d\n",
            i, (int)C.clr.A, (int)C.clr.R, (int)C.clr.G, (int)C.clr.B);
  }
  destroyPixFilt(F);
}
#endif //__PIXFILT_TEST__
