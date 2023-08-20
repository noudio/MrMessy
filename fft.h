#ifndef FFT_H
#define FFT_H

/* =========================================================
*/
struct complex {
    double R;
    double I;
};
typedef struct complex complex;

class Fft {

   public:

   // (size = 2^Power, Forwards: Dir == 0, FreqWindow == amount of convoluted bins)
   Fft(int Power, int Dir, double FreqWindow = 1.0);
   ~Fft();

   void Transform (complex *I, complex *O);
   void Show      (complex *O);
   void Dump      (void);

   int       FftNu;
   int       FftSize;
   double    FftFact;

protected:
   // data
   double   *Window;
   complex  *FftW;
   complex  *FftWbr;
   complex  *h1;
   complex  *h2;
   int      *bit_idx;
   int       dir;

   // ---------------------------------------
   // checking, using known good algorithm (DitNmRd2)
   double CheckTransform  (complex *I, complex *O);

   // ---------------------------------------
   // transform algorithms

   void TransformDitNmRd2 (complex *I, complex *O); // decimation-in-time normal-order radix-2
   void TransformDitDrRd2 (complex *I, complex *O); // decimation-in-time digit-reversed-order radix-2
   void TransformDifNmRd2 (complex *I, complex *O); // decimation-in-time normal-order radix-2
   void TransformDifDrRd2 (complex *I, complex *O); // decimation-in-time digit-reversed-order radix-2

   // ---------------------------------------
   // butterfly op's : decimation-in-time

   inline void   BflyDitRd2   (complex *I0,
                               complex *I1,
                               complex *O0,
                               complex *O1,
                               complex *w);

   inline void   BflyDitRd2W0 (complex *I0,
                               complex *I1,
                               complex *O0,
                               complex *O1);

   inline void   BflyDitRd2Wj (complex *I0,
                               complex *I1,
                               complex *O0,
                               complex *O1);

   // ---------------------------------------
   // butterfly op's : decimation-in-frequency
   inline void   BflyDifRd2   (complex *I0,
                               complex *I1,
                               complex *O0,
                               complex *O1,
                               complex *w);

   inline void   BflyDifRd2W0 (complex *I0,
                               complex *I1,
                               complex *O0,
                               complex *O1);

   // ---------------------------------------
   // window functions

   void   KaiserWindow (int Length,  double Transitionwidth, double *Window);
   double KaiserOrder  (int Length,  double TransitionWidth);
   double Kaiser       (double Beta, double T,               double t);
   double I0           (double x);

   // ---------------------------------------
   // infra

   void   Error   (int x, char *fmt, ...);
   void   Warning (char *fmt, ...);
};



#endif
