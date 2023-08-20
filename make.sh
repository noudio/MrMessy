#!/bin/bash -x
#g++ xl01.cpp -o xl01 -lX11
#gcc viewer.c -o viewer -lX11 -lXext
#gcc viewer_noud.c -o viewer_noud -lX11 -lXext
# the project

odir="./obj"
flgs="-g"
mkdir -p $odir

#objects
g++ $flgs -O3 -o $odir/fft.o      -c fft.cc        # -lX11 -lXext
g++ $flgs -o $odir/pixfilt.o      -c pixfilt.c    # -lX11 -lXext
g++ $flgs -o $odir/xdisp.o        -c xdisp.c      # -lX11 -lXext
#gcc -pg -o $odir/jpegdecode.o    -c jpegdecode.c # -ljpeg
#gcc -pg -o $odir/processq.o      -c processq.c   # -lpthread
g++ $flgs -o $odir/noudioJack.o   -c noudioJack.c    # -lX11 -lXext
g++ $flgs -o $odir/specselected.o -c specselected.c  -Wno-trigraphs
g++ $flgs -o $odir/machine.o      -c machine.cc
g++ $flgs -o $odir/xdisp_main.o   -c xdisp_main.cc # -lX11 -lXext

#final linkage
g++ $flgs -o $odir/xdisp \
    $odir/xdisp.o \
    $odir/xdisp_main.o \
    $odir/pixfilt.o \
    $odir/specselected.o \
    $odir/machine.o \
    $odir/noudioJack.o \
    $odir/fft.o \
    -lX11 -lXext -ljack -lm

#gcc -pg $odir/jpegdecode_main.c -o jpegdecode jpegdecode.o xdisp.o pixfilt.o -ljpeg -lX11 -lXext
# gcc -g -o $odir/noudioJack $odir/noudioJack.o -ljack -lm

#cp
cp $odir/xdisp xdisp
cp $odir/noudioJack noudioJack
