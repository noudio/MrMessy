#ifndef FFT_C
#define FFT_C

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "fft.h"

#undef FFT_OLD_MALLOC

void Fft::Error (int x, char *fmt, ...)
{
   return;
}

void Fft::Warning (char *fmt, ...)
{
   return;
}

// ===================================================================================================================

#define  FftMaxNu    (20)
#define  FftMaxSize  (1<<FftMaxNu)

// ===================================================================================================================

Fft::Fft (int Power, int Dir, double FreqWindow)
{
   double  Phi, Ghi;
   int     i;
   int     Wsize;

   if (Power < 1 || Power > FftMaxNu)
      Error(1, (char *)"parameter illegal.\n");

   FftNu = Power;
   FftSize = 1 << FftNu;

   dir = (Dir != 0);
   if (dir) {
       // Dir is 1: Twiddle factor rotate forwards: inverse fft
       Dir     = 1;
       FftFact = 1.0;
   }
   else {
       // Dir is -1: Twiddle factor rotate backwards: forward fft
       Dir     = -1;
       FftFact = 1.0 / (double)FftSize;


       // factor: 1/FftSize       :: analyze bins to wave amps. wave = bin[i]*ejw[i]n
       // factor: 1/sqrt(FftSize) :: analyze bins to rms amps
       // factor: 1               :: energy conservative fft
   }

   // ------------------------------------------------------------------------------------------------------
   // initialize work vectors

   h1      = new complex[FftSize];
   h2      = new complex[FftSize];
   Window  = new double[FftSize];
   bit_idx = new int[FftSize];

   if (!h1 || !h2 || !Window || bit_idx)
       Error(1, (char *)"Cannot allocate memory\n");

   KaiserWindow (FftSize, FreqWindow/(double)FftSize, Window);

   for (i=0; i<FftSize; i++)
      Window[i] *= FftFact;

   for (i=0; i<FftSize; i++) {

      int j, k0, k1;

      /* Create reversed index
      */
      for (j=0, k0=0, k1=i; j<FftNu; j++) {
         k0 <<=1;
         k0 |= k1 & 1 ? 1 : 0;
         k1 >>=1;
      }

      bit_idx[i] = k0;
   }

   // ------------------------------------------------------------------------------------------------------
   // initialize twiddle factors, 2 sets

   // [A] One always need only half of the Twiddle factors
   // (the others are negatives, of the first half ((, which is what butterflies are about: combining this)))
   Wsize = FftSize >> 1;

   // [B] alloc
   FftW   = new complex[Wsize];  // normal order over n/2
   FftWbr = new complex[Wsize];  // bit reversed order over n/2
   if (!FftW || !FftWbr)
       Error(1,(char *)"Cannot allocate mem for FftW\n");
   #ifdef DBG
   Warning("FftW   = %p\n", FftW);
   Warning("FftWbr = %p\n", FftWbr);
   #endif

   // [C] initialize normal twiddles:: For normal fft, they run backwards in phase 0...n/2-1 ==> 0... - pi *(1-2/n)
   Phi = (double)Dir * M_PI / (double)(Wsize);
   for (i=0; i<Wsize; i++) {

       Ghi = Phi * (double)(i); // explicitly no addition, to prevent phase accumulation errors
       FftW[i].R = cos(Ghi);
       FftW[i].I = sin(Ghi);
   }

   #ifdef DBG
   fprintf(stderr, "\n\nW factors...\n");
   Show(FftW, Wsize);
   #endif

   // [D] initialize bit reversed twiddles, they are bit-reversed over the range n/2
   for (i=0; i<Wsize; i++) {

       // NOTE: bit reversed order{ over range n/2} == [1/2] * bit reversed order { over range n }
       FftWbr[i] = FftW[bit_idx[i]>>1];
   }
}

// ===================================================================================================================

Fft::~Fft()
{
    if (FftWbr)  {delete[] FftWbr;  FftWbr  = 0;}
    if (FftW)    {delete[] FftW;    FftW    = 0;}
    if (h1)      {delete[] h1;      h1      = 0;}
    if (h2)      {delete[] h2;      h2      = 0;}
    if (Window)  {delete[] Window;  Window  = 0;}
    if (bit_idx) {delete[] bit_idx; bit_idx = 0;}
}

// ===================================================================================================================

void Fft::Show (complex *I)
{
   int i;
   complex val;

   for (i=0; i<FftSize ; i++) {

      val = I[i];

      fprintf (stdout, " [%4u] : (%9.6g  %9.6g)\n", i,  val.R, val.I);
   }
}


// ===================================================================================================================
//  Transform: Execute a suitable transform, test all of them alternatively

int     fft_test_id  = 0;
int     fft_test_cnt = 0;
double  fft_test_err = 0.0;

complex fft_ref_out[65536];

double Fft::CheckTransform(complex *I, complex *O)
{
    // take reference FFT
    TransformDitNmRd2(I,fft_ref_out);

    // analyze output
    int     i;
    complex *o = O;
    complex *r = fft_ref_out;
    complex e;
    double  pr = 0.0;
    double  pe = 0.0;
    for (i=0; i<FftSize; i++) {
        e.R = o->R - r->R;
        e.I = o->I - r->I;

        pr += r->R*r->R + r->I*r->I;
        pe += e.R*e.R   + e.I*e.I;
    }
    return pe/pr;
}


void Fft::Transform (complex *I, complex *O)
{
    // ===================================
    // take algorithm of choice
    TransformDifNmRd2(I,O);
    //TransformDifDrRd2(I,O);

    return;

    // ===================================
    // more elaborate schemes
    fft_test_id = 3;
    switch(fft_test_id) {
        default:
        case 0:
            TransformDitNmRd2(I,O); //  0.76150 ms 8K-fft [n=1000]
            break;
        case 1:
            TransformDitDrRd2(I,O); //  0.80847 ms 8K-fft [n=1000]
            break;
        case 2:
            TransformDifNmRd2(I,O); //  0.59778 ms 8K-fft [n=1000]
            break;
        case 3:
            TransformDifDrRd2(I,O); //  0.62611 ms 8K-fft [n=1000]
            break;
    }
    fft_test_id = (fft_test_id + 1) % 4;

/*
    fft_test_err += CheckTransform(I, O);
    fft_test_cnt ++;

    if ((fft_test_cnt % 1000)==0) {
        double edb = 10.0*log10(fft_test_err/(double)fft_test_cnt);
        fprintf(stderr, "fft_err:: %7.2f[dB]\n", edb);
        fft_test_cnt = 0;
        fft_test_err = 0.0;
    }
*/

}

// ===================================================================================================================
//  Transform 1: classic decimation-in-time dft combining
//    * bit reversed in
//    * combine dft outputs of the odd input samples with twiddles to the even dft output
//    * normal order out

void Fft::TransformDitNmRd2 (complex *I, complex *O)
{
   double  *W;
   complex *src;
   complex *tgt;
   int i;
   int g;
   int j0, j1;
   int s0, s1;
   int j0step, pstep, p;
   int *idx;

   // ----------------------------------------------------------------------------------------------------------------
   // 1: Permutate and size input to reversed bit index
   //    Normal (dual double) representation is used

#ifdef DBG
   printf("permute and scale:\n");
#endif

   idx = bit_idx;
   W   = Window;

   for (i=0; i<FftSize; i++) {

      h1[*idx].R = (*W) * I->R;
      h1[*idx].I = (*W) * I->I;

      W++;
      I++;
      idx++;
   }
#ifdef DBG
   fprintf(stderr, "input samples:\n");
   Show(h1, FftSize);
#endif

   // ----------------------------------------------------------------------------------------------------------------
   // 2: Butterfly steps (src starts always at h1)

   //int p4 = (1<<(FftNu-2));

   for (i=0; i<FftNu; i++) {

#ifdef DBG
      printf ("butterfly %u   ", i);
#endif

      // -------------------------------------------------------------------------------------------------------------
      // Point out Src & Tgt Arrays
      //   As a butterfly works 'index_in_place' a different Src and Tgt array are in fact not needed
      //   However it may be worthwhile to keep them different for optimizing on different platforms.
      //   Think of caching! (source,target is always addressed linearly)
      //   If the target and source are the same: Then the butterfly must be sure to calculate the output
      //   first before storing. (i.e. not accessing the target for intermediate values)

      // The source array "src" will alternate between the two
      //   work arrays "h1" and "h2", (of course src starts with h1 and tgt with h2)
      //   The target array "tgt" is the opposite work array
      //   as the source during intermediate steps

      src = i & 1 ? h2 : h1;
      tgt = i & 1 ? h1 : h2;

      // optimization: for the last stage, the tgt is directoy the output buffer
      if (i==FftNu-1) tgt = O;

#ifdef DBG
      printf ("%-2s ==> %-2s\n",
          (src == h1 ? "h1" : "h2"),
          (tgt == h1 ? "h1" : (tgt == h2 ? "h2" : "O")));
#endif
      // -------------------------------------------------------------------------------------------------------------
      // Butterfly steps 1 2 4 8 ... 1<<i
      g      = 1 << i;           // 1,   2,   4,   8,    ... N/2
      j0step = g << 1;           // 2,   4,   8,   16,   ... N
      pstep  = 1<<(FftNu-1-i);   // N/2, N/4, N/8, N/16, ... 1

      // every stage performs in total N/2 butterfly steps (each butterfly processing 2 samples: N total samples)
      // stage i=0   : j1:: 0,2,4,....N-2  (N/2  steps) j2:: 0         (1 Step  g) p:: 0  (repeat N/2)       == w[0]_2  _  w[0]_2 _ ...          (s1,s2)=>(0,1)_(2,3)_(4,5)_(6,7)_...
      // stage i=1   : j1:: 0,4,8,....N-4  (N/4  steps) j2:: 0,1       (2 Steps g) p:: 0, 1*N/4              == w[0]_4  !  w[1]_4  _ w[0]_4 ...  (s1,s2)=>(0,2)!(1,3)_(4,6)!(5,7)_....
      // stage i=2   : j1:: 0,8,16,...N-8  (N/8  steps) j2:: 0,1..3    (4 Steps g) p:: 0, 1*N/8,  ... 3*N/8  == w[0]_8  !! w[3]_8  _ w[0]_8 ...  (s1,s2)=>(0,4)!(1,5)!!(3,7)_(8,12)!(9,13)!...
      // stage i=3   : j1:: 0,16,32...N-16 (N/16 steps) j2:: 0,1..7    (8 Steps g) p:: 0, 1*N/16, ... 7*N/16 == w[0]_16 !! w[7]_16 _ w[0]_16 ... (s1,s2)=>(0,8)!(1,9)!!(7,15)_(16,24)...
      // ...
      // stage i=N-1 : j1:: 0              (1    step)  j2:: 0...N/2-1 (N/2 steps) p:: 0, 1, 2    ... N/2-1  == w[0]_N  !! w[N/2-1]_N        ... (s1,s2)=>(0,N/2)!(1,N/2+1)!...!(N/2-1,N-1)
      // stage 0 and 1 could be optimized by using special butterflies that don't require complex multiplications
      // -------------------------------------------------------------------------------------------------------------

      complex *src_0_j0stride = src;
      complex *src_1_j0stride = src + g;
      complex *tgt_0_j0stride = tgt;
      complex *tgt_1_j0stride = tgt + g;

      for     (j0 = 0; j0 < FftSize ; j0 += j0step) {

          p=0;

          complex *src_0_j1stride = src_0_j0stride;
          complex *src_1_j1stride = src_1_j0stride;
          complex *tgt_0_j1stride = tgt_0_j0stride;
          complex *tgt_1_j1stride = tgt_1_j0stride;
          complex *fftw_j1stride  = FftW;

          for (j1 = 0; j1 < g ; /*j1++*/) {

              // s0 = j0+j1;
              // s1 = s0 ^ g;
              // BflyFd(&src[s0], &src[s1], &tgt[s0], &tgt[s1], &FftW[p]);
              // #ifdef DBG
              // printf ("      (%4u, %4u) ^ %u\n", s0, s1, p);
              // #endif

              if (p==0) {
                  BflyDitRd2W0(src_0_j1stride,
                               src_1_j1stride,
                               tgt_0_j1stride,
                               tgt_1_j1stride);
              }
/*
              else if (p == p4) {
                  BflyDitRd2Wj(src_0_j1stride,
                               src_1_j1stride,
                               tgt_0_j1stride,
                               tgt_1_j1stride);
              }
*/
              else {
                  BflyDitRd2(src_0_j1stride,
                             src_1_j1stride,
                             tgt_0_j1stride,
                             tgt_1_j1stride,
                             fftw_j1stride);
              }

             j1++;
             if (j1 < g) {
                  src_0_j1stride++;
                  src_1_j1stride++;
                  tgt_0_j1stride++;
                  tgt_1_j1stride++;
                  fftw_j1stride+=pstep;
                  p+=pstep;
             }
             // else
             //     break;
          }

          src_0_j0stride += j0step;
          src_1_j0stride += j0step;
          tgt_0_j0stride += j0step;
          tgt_1_j0stride += j0step;
      }
#ifdef DBG
      fprintf(stderr, "\n\nStage %d\n", i);
      Show(tgt, FftSize);
#endif
   }

   // ----------------------------------------------------------------------------------------------------------------
   // Copy to output (and convert to output representation if required)
   // this last step can be optimized away by proper choosing the tgt array
   //for (i=0; i<FftSize; i++)
   //   *O++ = *tgt++;
}

// ===================================================================================================================
//  Transform 2: time decimation dft combining, radix-2 but in bit reversed equation/processing order
//    * normal order in
//    * still combine dft outputs of the odd input samples with twiddles to the even samples dft output
//    * (but the equation order is bit reversed)
//    * the stages have reversed stride distances and steps (j0,j1 inside out)
//    * the twiddle factors go in bit reversed order
//    * bit reversed order out

void Fft::TransformDitDrRd2 (complex *I, complex *O)
{
    double  *W;
    complex *src;
    complex *tgt;
    int i;
    int g;
    int j0, j1;
    int s0, s1;
    int j0step, pstep, p;
    int *idx;

    // ----------------------------------------------------------------------------------------------------------------
    // 1: Scale input

    #ifdef DBG
    printf("scale:\n");
    #endif

    W   = Window;
    tgt = h1;

    for (i=0; i<FftSize; i++) {

        tgt->R = (*W) * I->R;
        tgt->I = (*W) * I->I;

        W++;
        I++;
        tgt++;
    }
    #ifdef DBG
    fprintf(stderr, "input samples:\n");
    Show(h1, FftSize);
    #endif

    // ----------------------------------------------------------------------------------------------------------------
    // 2: Butterfly steps (src starts always at h1)

    for (i=0; i<FftNu; i++) {

        #ifdef DBG
        printf ("butterfly %u   ", i);
        #endif

        // -------------------------------------------------------------------------------------------------------------
        // Point out Src & Tgt Arrays
        //   As a butterfly works 'index_in_place' a different Src and Tgt array are in fact not needed
        //   However it may be worthwhile to keep them different for optimizing on different platforms.
        //   Think of caching! (source,target is always addressed linearly)
        //   If the target and source are the same: Then the butterfly must be sure to calculate the output
        //   first before storing. (i.e. not accessing the target for intermediate values)

        // The source array "src" will alternate between the two
        //   work arrays "h1" and "h2", (of course src starts with h1 and tgt with h2)
        //   The target array "tgt" is the opposite work array
        //   as the source during intermediate steps

        src = i & 1 ? h2 : h1;
        tgt = i & 1 ? h1 : h2;

        #ifdef DBG
        printf ("%-2s ==> %-2s\n",
                (src == h1 ? "h1" : "h2"),
                (tgt == h1 ? "h1" : (tgt == h2 ? "h2" : "O")));
        #endif
        // -------------------------------------------------------------------------------------------------------------
        // Butterfly steps 1 2 4 8 ... 1<<i
        //g      = 1 << i;           // 1,   2,   4,   8,    ... N/2
        //j0step = g << 1;           // 2,   4,   8,   16,   ... N
        //pstep  = 1<<(FftNu-1-i);   // N/2, N/4, N/8, N/16, ... 1

        g      = 1<<(FftNu-1-i);   // N/2, N/4, ...            1
        j0step = g << 1;           // N,   N/2. ...            2
        pstep  = 1;

        // every stage performs in total N/2 butterfly steps (each butterfly processing 2 samples: N total samples)
        // ...
        // stage i=0   : j1:: 0              (1    step)  j2:: 0...N/2-1 (N/2 steps) p:: 0  (repeat N/2)
        // stage i=1   : j1:: 0,N/2          (2    steps) j2:: 0...N/2-1 (N/4 steps) p:: 0, (repeat N/4) br(1) (repeat N/4)
        // ...
        // stage i=N-1 : j1:: 0,2,4,....N-2  (N/2  steps) j2:: 0         (1 Step  g) p:: 0,1,2,3,4 .. N/2-1

        // stage 0 and 1 could be optimized by using special butterflies that don't require complex multiplications

        complex *src_0_j0stride = src;
        complex *src_1_j0stride = src + g;
        complex *tgt_0_j0stride = tgt;
        complex *tgt_1_j0stride = tgt + g;
        complex *fftw_j0stride  = FftWbr;   // twiddles in bit reversed order, and stepped outside j1

        for     (j0 = 0, p=0;   j0 < FftSize ; j0 += j0step, p++) {

            complex *src_0_j1stride = src_0_j0stride;
            complex *src_1_j1stride = src_1_j0stride;
            complex *tgt_0_j1stride = tgt_0_j0stride;
            complex *tgt_1_j1stride = tgt_1_j0stride;

            for (j1 = 0;        j1 < g       ; j1++) {

                //if (p==0) {
                //    BflyDitRd2W0(src_0_j1stride,
                //                 src_1_j1stride,
                //                 tgt_0_j1stride,
                //                 tgt_1_j1stride);
                //}
                //else {
                    BflyDitRd2(src_0_j1stride,
                               src_1_j1stride,
                               tgt_0_j1stride,
                               tgt_1_j1stride,
                               fftw_j0stride);
                //}

                src_0_j1stride++;
                src_1_j1stride++;
                tgt_0_j1stride++;
                tgt_1_j1stride++;
            }

            src_0_j0stride += j0step;
            src_1_j0stride += j0step;
            tgt_0_j0stride += j0step;
            tgt_1_j0stride += j0step;
            fftw_j0stride++;
        }
        #ifdef DBG
        fprintf(stderr, "\n\nStage %d\n", i);
        Show(tgt, FftSize);
        #endif
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Copy to output (and convert to output representation if required)
    // this last step can be optimized away by proper choosing the tgt array
    idx = bit_idx;
    for (i=0; i<FftSize; i++)
       *O++ = tgt[*idx++];   // *tgt++;
}

// ===================================================================================================================
// Radix2 Time Decimation butterfly
// inline local variables, expressions arranged for optimal performance -O3

inline void Fft::BflyDitRd2 (complex *I0,
                             complex *I1,
                             complex *O0,
                             complex *O1,
                             complex *w)
{
    double o0r = I1->R * w->R - I1->I * w->I;
    double o0i = I1->R * w->I + I1->I * w->R;

    O1->R = -o0r + I0->R;
    O1->I = -o0i + I0->I;

    O0->R =  o0r + I0->R;
    O0->I =  o0i + I0->I;
}

// ===================================================================================================================
// Time Decimation butterfly in case *w = w[0]_n
// somehow only computational gain for normal order time decimation.
// maybe we have to do loop/stage unrolling

inline void Fft::BflyDitRd2W0 (complex *I0,
                               complex *I1,
                               complex *O0,
                               complex *O1)
{
    O1->R = -I1->R + I0->R;
    O1->I = -I1->I + I0->I;

    O0->R =  I1->R + I0->R;
    O0->I =  I1->I + I0->I;
}

// ===================================================================================================================
// Time Decimation butterfly in case *w = w[n/4]_n == -j (forwards fft) or j (ifft)
// (it is not used, no computational gain came from that (yet), maybe loop/stage unrolling helps)

inline void Fft::BflyDitRd2Wj (complex *I0,
                               complex *I1,
                               complex *O0,
                               complex *O1)
{
    if (dir) {
        O1->R = -I1->I + I0->R;
        O1->I =  I1->R + I0->I;

        O0->R =  I1->I + I0->R;
        O0->I = -I1->R + I0->I;
    }
    else {
        O1->R =  I1->I + I0->R;
        O1->I = -I1->R + I0->I;

        O0->R = -I1->I + I0->R;
        O0->I =  I1->R + I0->I;
    }
}


// ===================================================================================================================
//  Transform 3: classic frequency decimation dft combining, radix-2 normal order
//    * bit reversed order in -> normal order out
//    * combine dft of the 2nd half input samples with twiddles to the 1st half input samples
//    * evaluate to even and odd dft outputs

//    * the stage structuring is almost the same as DIT bitreversed, except that normal order
//      twiddle factors are being used, and stepped in the inner loop, and that an inversed
//      (dual) butterfly is applied

void Fft::TransformDifNmRd2 (complex *I, complex *O)
{
    double  *W;
    complex *src;
    complex *tgt;
    int i;
    int g;
    int j0, j1;
    int s0, s1;
    int j0step, pstep, p;
    int *idx;

    // ----------------------------------------------------------------------------------------------------------------
    // 1: Scale input

    #ifdef DBG
    printf("scale:\n");
    #endif

    W   = Window;
    tgt = h1;

    for (i=0; i<FftSize; i++) {

        tgt->R = (*W) * I->R;
        tgt->I = (*W) * I->I;

        W++;
        I++;
        tgt++;
    }
    #ifdef DBG
    fprintf(stderr, "input samples:\n");
    Show(h1, FftSize);
    #endif

    // ----------------------------------------------------------------------------------------------------------------
    // 2: Butterfly steps (src starts always at h1)

    for (i=0; i<FftNu; i++) {

        #ifdef DBG
        printf ("butterfly %u   ", i);
        #endif

        // -------------------------------------------------------------------------------------------------------------
        // Point out Src & Tgt Arrays
        //   As a butterfly works 'index_in_place' a different Src and Tgt array are in fact not needed
        //   However it may be worthwhile to keep them different for optimizing on different platforms.
        //   Think of caching! (source,target is always addressed linearly)
        //   If the target and source are the same: Then the butterfly must be sure to calculate the output
        //   first before storing. (i.e. not accessing the target for intermediate values)

        // The source array "src" will alternate between the two
        //   work arrays "h1" and "h2", (of course src starts with h1 and tgt with h2)
        //   The target array "tgt" is the opposite work array
        //   as the source during intermediate steps

        src = i & 1 ? h2 : h1;
        tgt = i & 1 ? h1 : h2;

        #ifdef DBG
        printf ("%-2s ==> %-2s\n",
                (src == h1 ? "h1" : "h2"),
                (tgt == h1 ? "h1" : (tgt == h2 ? "h2" : "O")));
        #endif
        // -------------------------------------------------------------------------------------------------------------
        // Butterfly steps 1 2 4 8 ... 1<<i
        g      = 1<<(FftNu-1-i);   // N/2, N/4, ...    1
        j0step = g << 1;           // N,   N/2. ...    2
        pstep  = 1 << i;           // 1, 2, 4,  ...    N/2

        // every stage performs in total N/2 butterfly steps (each butterfly processing 2 samples: N total samples)
        // ...
        // stage i=0   : j1:: 0              (1    step)  j2:: 0...N/2-1 (N/2 steps) p:: 0,1,2,3,4 .. N/2-1
        // stage i=1   : j1:: 0,N/2          (2    steps) j2:: 0...N/2-1 (N/4 steps) p:: 0,2,4        N/2-2 (repeat 2)
        // ...
        // stage i=N-1 : j1:: 0,2,4,....N-2  (N/2  steps) j2:: 0         (1 Step  g) p:: 0,0,0,0,0 (repeat N/2)

        // stage N-1 and N-2 could be optimized by using special butterflies that don't require complex multiplications

        complex *src_0_j0stride = src;
        complex *src_1_j0stride = src + g;
        complex *tgt_0_j0stride = tgt;
        complex *tgt_1_j0stride = tgt + g;

        for     (j0 = 0;             j0 < FftSize ; j0 += j0step) {

            complex *src_0_j1stride = src_0_j0stride;
            complex *src_1_j1stride = src_1_j0stride;
            complex *tgt_0_j1stride = tgt_0_j0stride;
            complex *tgt_1_j1stride = tgt_1_j0stride;
            complex *fftw_j1stride  = FftW;

            for (j1 = 0 /*, p=0*/;        j1 < g       ; j1++ /*, p+=pstep*/) {

                //if (p==0) {
                //    BflyDifRd2W0(src_0_j1stride,
                //                 src_1_j1stride,
                //                 tgt_0_j1stride,
                //                 tgt_1_j1stride);
                //}
                //else {
                    BflyDifRd2(src_0_j1stride,
                               src_1_j1stride,
                               tgt_0_j1stride,
                               tgt_1_j1stride,
                               fftw_j1stride);
                //}

                src_0_j1stride++;
                src_1_j1stride++;
                tgt_0_j1stride++;
                tgt_1_j1stride++;
                fftw_j1stride+=pstep;
            }

            src_0_j0stride += j0step;
            src_1_j0stride += j0step;
            tgt_0_j0stride += j0step;
            tgt_1_j0stride += j0step;
        }
        #ifdef DBG
        fprintf(stderr, "\n\nStage %d\n", i);
        Show(tgt, FftSize);
        #endif
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Copy to output (and convert to output representation if required)
    // this last step can be optimized away by proper choosing the tgt array
    idx = bit_idx;
    for (i=0; i<FftSize; i++)
        *O++ = tgt[*idx++];   // *tgt++;
}

// ===================================================================================================================
//  Transform 4: frequency decimation dft combining, radix-2 bit-reversed order
//    * normal order in -> bit-reversed order out
//    * Still combine dft of the 2nd half input samples with twiddles to the 1st half input samples
//    * evaluate to even and odd dft outputs, but processing is in bit-reversed order

//    * the stage structuring is almost the same as DIT normal-order, except that bit-reversed order
//      twiddle factors are being used, and stepped in the outer loop, and that an inversed
//      (dual) butterfly is applied

void Fft::TransformDifDrRd2 (complex *I, complex *O)
{
    double  *W;
    complex *src;
    complex *tgt;
    int i;
    int g;
    int j0, j1;
    int s0, s1;
    int j0step, pstep, p;
    int *idx;

    // ----------------------------------------------------------------------------------------------------------------
    // 1: Permutate and size input to reversed bit index
    //    Normal (dual double) representation is used

    #ifdef DBG
    printf("permute and scale:\n");
    #endif

    idx = bit_idx;
    W   = Window;

    for (i=0; i<FftSize; i++) {

        h1[*idx].R = (*W) * I->R;
        h1[*idx].I = (*W) * I->I;

        W++;
        I++;
        idx++;
    }
    #ifdef DBG
    fprintf(stderr, "input samples:\n");
    Show(h1, FftSize);
    #endif

    // ----------------------------------------------------------------------------------------------------------------
    // 2: Butterfly steps (src starts always at h1)

    //int p4 = (1<<(FftNu-2));

    for (i=0; i<FftNu; i++) {

        #ifdef DBG
        printf ("butterfly %u   ", i);
        #endif

        // -------------------------------------------------------------------------------------------------------------
        // Point out Src & Tgt Arrays
        //   As a butterfly works 'index_in_place' a different Src and Tgt array are in fact not needed
        //   However it may be worthwhile to keep them different for optimizing on different platforms.
        //   Think of caching! (source,target is always addressed linearly)
        //   If the target and source are the same: Then the butterfly must be sure to calculate the output
        //   first before storing. (i.e. not accessing the target for intermediate values)

        // The source array "src" will alternate between the two
        //   work arrays "h1" and "h2", (of course src starts with h1 and tgt with h2)
        //   The target array "tgt" is the opposite work array
        //   as the source during intermediate steps

        src = i & 1 ? h2 : h1;
        tgt = i & 1 ? h1 : h2;

        // optimization: for the last stage, the tgt is directoy the output buffer
        if (i==FftNu-1) tgt = O;

        #ifdef DBG
        printf ("%-2s ==> %-2s\n",
                (src == h1 ? "h1" : "h2"),
                (tgt == h1 ? "h1" : (tgt == h2 ? "h2" : "O")));
        #endif
        // -------------------------------------------------------------------------------------------------------------
        // Butterfly steps 1 2 4 8 ... 1<<i
        g      = 1 << i;           // 1,   2,   4,   8,    ... N/2
        j0step = g << 1;           // 2,   4,   8,   16,   ... N
        pstep  = 1;                // 1,   1,   1,

        // every stage performs in total N/2 butterfly steps (each butterfly processing 2 samples: N total samples)
        // stage i=0   : j1:: 0,2,4,....N-2  (N/2  steps) j2:: 0         (1 Step  g) p:: br{0,1,2,3,4 ...N/2-1}
        // ...
        // stage i=N-1 : j1:: 0              (1    step)  j2:: 0...N/2-1 (N/2 steps) p:: 0,0,0,0 {repeat N/2}

        // stage N-1 and N-2 could be optimized by using special butterflies that don't require complex multiplications
        // -------------------------------------------------------------------------------------------------------------

        complex *src_0_j0stride = src;
        complex *src_1_j0stride = src + g;
        complex *tgt_0_j0stride = tgt;
        complex *tgt_1_j0stride = tgt + g;
        complex *fftw_j0stride  = FftWbr;

        for     (j0 = 0; j0 < FftSize ; j0 += j0step) {

            complex *src_0_j1stride = src_0_j0stride;
            complex *src_1_j1stride = src_1_j0stride;
            complex *tgt_0_j1stride = tgt_0_j0stride;
            complex *tgt_1_j1stride = tgt_1_j0stride;

            for (j1 = 0; j1 < g ; j1++) {

                BflyDifRd2(src_0_j1stride,
                           src_1_j1stride,
                           tgt_0_j1stride,
                           tgt_1_j1stride,
                           fftw_j0stride);

                src_0_j1stride++;
                src_1_j1stride++;
                tgt_0_j1stride++;
                tgt_1_j1stride++;
            }

            src_0_j0stride += j0step;
            src_1_j0stride += j0step;
            tgt_0_j0stride += j0step;
            tgt_1_j0stride += j0step;
            fftw_j0stride  += pstep; // == 1
        }
        #ifdef DBG
        fprintf(stderr, "\n\nStage %d\n", i);
        Show(tgt, FftSize);
        #endif
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Copy to output (and convert to output representation if required)
    // this last step can be optimized away by proper choosing the tgt array
    //for (i=0; i<FftSize; i++)
    //   *O++ = *tgt++;
}

// ===================================================================================================================
// Radix2 Frequency Decimation butterfly

inline void Fft::BflyDifRd2 (complex *I0,
                             complex *I1,
                             complex *O0,
                             complex *O1,
                             complex *w)
{
    // intermediate
    double o1r = I0->R - I1->R;
    double o1i = I0->I - I1->I;

    O0->R = I0->R + I1->R;
    O0->I = I0->I + I1->I;

    // rotate in w
    O1->R = o1r * w->R - o1i * w->I;
    O1->I = o1r * w->I + o1i * w->R;
}


// ===================================================================================================================
// Radix2 Frequency Decimation butterfly
// no computational gain for normal order frequency decimation.
// maybe we have to do loop/stage unrolling

inline void Fft::BflyDifRd2W0 (complex *I0,
                               complex *I1,
                               complex *O0,
                               complex *O1)
{
    O1->R = I0->R - I1->R;
    O1->I = I0->I - I1->I;
    O0->R = I0->R + I1->R;
    O0->I = I0->I + I1->I;
}

// ===================================================================================================================

void Fft::Dump (void)
{
    fprintf(stderr,"FFT object %p:\n"
    "\tFftNu:   %d\n"
    "\tFftSize: %d\n"
    "\tFftFact: %f\n"
    "\tFftW:    %p\n"
    "\th1:      %p\n"
    "\th2:      %p\n"
    "", this, FftNu, FftSize, FftFact, FftW, h1, h2);
}

// ===================================================================================================================
// Calculate the complete KaiserWindow

void Fft::KaiserWindow (int    Length,          // Total bins
                        double Transitionwidth, // Width of frequency response transient as a fraction of 1.0 (total response)
                        double *Window)         // output storage
{
    double Beta;
    int    i;
    double t;


    Beta = KaiserOrder(Length, Transitionwidth);

    // t runs from -[L/2-1/2] ... +[L/2-1/2]
    for (i=0; i<Length; i++) {

        t = (double)i - (double)(Length-1)*0.5;
        *Window++ = Kaiser(Beta, Length, t);
    }
}

// ===================================================================================================================
// Determine the Beta factor of the Kaiser Window, based upon the width of the transient frequency response

double Fft::KaiserOrder(int    Length,           // Total bins
                        double TransitionWidth)  // Width of frequency response transient as a fraction of 1.0 (total response)
{
    double A;     // Main lobe to side lobe  [dB]
    double Delta; // Peak approx error       [amp ratio]
    double Beta;  // Kaiser shape factor     [J0(x) function, x domain compression factor]

   A = (double)(Length-1)*2.285*TransitionWidth*M_PI + 8.0;

   Delta = exp(M_LN10*-A/20.0);

   if (A>50.0)
      Beta = 0.1102*(A - 8.7);
   else
   if (A>=21.0)
      Beta = 0.5842*pow(A-21.0, 0.4) + 0.07886*(A-21.0);
   else
      Beta = 0;

//   printf("# Kaiser     A: %lf  (main lobe to side lobe dB)\n", A);
//   printf("# Kaiser Delta: %lf  (peak approx error)\n", Delta);
//   printf("# Kaiser  Beta: %lf  (Kaiser shape factor)\n", Beta);

   return Beta;
}

// ===================================================================================================================
// Calculate Kaiser Window coeeficient

double Fft::Kaiser(double Beta,   // Kaiser Shape factor
                   double T,      // T = L
                   double t)      // t e [-0.5(L-1)...0.5(L-1)]
{
    double t2;
    double I0norm;

    t2 = 2*t/T;
    t2 *= t2;
    if (t2 >= 1.0) return 0.0;
    // ==>
    // *= 2/T  = >-1 ...  0 ... <+1 (line)
    // *= t2   = <+1 ...  0 ... <+1 (parabola)

    I0norm = I0(Beta); // I0(Beta) at [t2 == 0]
    if (fabs(I0norm)<1.0e-13) return 1.0;

    // ==>
    // := 1-t2 = > 0 ... +1 ... > 0 (negated parabola)
    // := sqrt(1-t2)
    //        = > 0 ... +1 ... > 0 (1-abs(original t2), linear segments)
    return I0( Beta * sqrt(1.0-t2) ) / I0norm;
}

// ===================================================================================================================
// calculate I0 function, Bessel first order function

double Fft::I0 (double x)
{
   double D, DS, S;

   DS = 1.0;
   D  = 0.0;
   S  = 1.0;

   x *= x;

   do {
      D  += 2.0;
      DS *= x/(D*D);
      S  += DS;
   } while (fabs(DS) > 1.0e-10*fabs(S));

   return S;
}

// ===================================================================================================================
// TAF
#endif
