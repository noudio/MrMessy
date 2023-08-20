// Header for MulAcc routines
// (c)2013 Noudio
#ifndef __MULACC_H__
#define __MULACC_H__
// ---------------------------------------------------------
#include <stdint.h>

typedef struct {
    uint8_t  B;
    uint8_t  G;
    uint8_t  R;
    uint8_t  A;
} rgb32_t;

typedef union {
    uint32_t val;
    rgb32_t  fld;
} pix32_t;

// accRep_t and pix32_t equal for now...
typedef struct {
    uint16_t R;
    uint16_t G;
    uint16_t B;
    uint16_t A;
} acc64_t;
typedef union {
    uint64_t val;
    acc64_t  fld;
} accRep_t;

typedef accRep_t mulAccIn_t;
typedef accRep_t mulAccOut_t;

// internal use
inline void       mulAccInit  (mulAccOut_t *a);
inline mulAccIn_t mulAccInput (uint32_t X);
inline void       mulAcc      (mulAccOut_t *a, int8_t X, mulAccIn_t Y);
inline uint32_t   mulAccExit  (mulAccOut_t *a);

// -----------------------------------------------------
inline void    mulAccInit (mulAccOut_t *a)
{
   a->val = 0;
}
// -----------------------------------------------------
inline mulAccIn_t mulAccInput (uint32_t X)
{
   pix32_t  x;
   mulAccIn_t R;

   x.val = X;
   R.fld.A = x.fld.A;
   R.fld.R = x.fld.R;
   R.fld.G = x.fld.G;
   R.fld.B = x.fld.B;
   return R;
}
// -----------------------------------------------------
inline void    mulAcc (mulAccOut_t *a, int8_t X, mulAccIn_t Y)
{
   //a->fld.R += (uint16_t)((int16_t)X * Y.fld.R);
   //a->fld.G += (uint16_t)((int16_t)X * Y.fld.G);
   //a->fld.B += (uint16_t)((int16_t)X * Y.fld.B);

   a->val += X * Y.val;
}
// -----------------------------------------------------
inline uint32_t mulAccExit (mulAccOut_t *a)
{
   pix32_t R;
   uint16_t r,g,b;

   a->val = a->val >> 7;

   R.fld.A = (uint8_t)a->fld.A;
   R.fld.R = (uint8_t)a->fld.R;
   R.fld.G = (uint8_t)a->fld.G;
   R.fld.B = (uint8_t)a->fld.B;

   return R.val;
}


// -----------------------------------------------------
/*
inline uint32_t mulAccExit (mulAccOut_t *a)
{
   pix32_t R;
   uint16_t r,g,b;

   r=a->fld.R;
   g=a->fld.G;
   b=a->fld.B;

   r >>= 7;
   g >>= 7;
   b >>= 7;

   R.fld.R = (uint8_t)r;
   R.fld.G = (uint8_t)g;
   R.fld.B = (uint8_t)b;

   return R.val;
}
*/

#endif // __MULLACC_H__
