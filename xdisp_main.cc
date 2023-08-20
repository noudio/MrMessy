/* noudio...
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <math.h>
#include <time.h>


#include "specselected.h"
#include "machine.h"
#include "xdisp.h"
#include "noudioJack.h"
#include "fft.h"

#define BUF_SZ 65536
const int  buf_sz = BUF_SZ;
float buf1[BUF_SZ];
float buf2[BUF_SZ];

int processBufs(int sz);
int processBufsScope(int sz);

double G_clksecs = 0.0;
int    G_clknbr  = 0;

//void StretchHor (uint32_t *scaleFrame, int oWidth, int oHeight,
//                 uint32_t *videoFrame, int iWidth, int iHeight,
//                 int hDiv, int disp_y, int disp_h);

// ----------------------------------------------------------
int main (int argc, char ** argv)
{
    bool doScope   = false;
    bool doBlack   = false;
    bool doConnect = false; // if false: then use Qjackctl patch bay for automatic connections

    // scan args
    for (int i=1; i<argc; i++) {
        char *a = argv[i];
        // strip leading hyphens
        while (*a == '-')
            a++;

        if (!strcmp(a, "scope")) {
            doScope = true;
        }
        if (!strcmp(a, "black")) {
            doBlack = true;
        }
        if (!strcmp(a, "no-connect")) {
            doConnect = true;
        }
    }


    // --------------------------------------------------------------------------------
    // load details from the machine stock
    pMachineDetails M = &MachineHp8591a;
    if (doScope) {
        M = &MachinePm3295;
    }

    // ------------------------------------------------------------------------------
    // Modify image for right sizes

    int Hstretch = 0; // 1100; // 520; // 0; // 1100; //600;
    int Vstretch = 0; // 200; // 145; // 200; //600;

    if (doBlack) {
        Hstretch = 520;
        Vstretch = 145;
    }
    M = MachineHstretch(M, Hstretch); // refreshes M with a new copy!
    M = MachineVstretch(M, Vstretch); // refreshes M with a new copy!

    // ----------------------------------------------------------------------------
    // print display region

    printf("display outer       w x h = %u x %u\n", M->img_w, M->img_h);
    printf("display image       w x h = %u x %u\n", M->img_disp_w, M->img_disp_h);

    // ------------------------------------------------------------------------------
    // Initialize display

    if (doBlack) {

        // make it all black
        int imsz = M->img_w * M->img_h;
        for (int imsi=0; imsi<imsz; imsi++)
            M->img_data[imsi] = 0xff000000;

        // make red lines
        for (int imsi=0; imsi<M->img_disp_w; imsi++) {
            M->img_data[M->img_disp_y * M->img_w + M->img_disp_x + imsi] = 0xff800000;
            M->img_data[(M->img_disp_y-1) * M->img_w + M->img_disp_x + imsi] = 0xff800000;

            M->img_data[(M->img_disp_y + M->img_disp_h)*M->img_w + M->img_disp_x+imsi] = 0xff800000;
            M->img_data[(M->img_disp_y+1 + M->img_disp_h)*M->img_w + M->img_disp_x+imsi] = 0xff800000;
        }

        for (int imsi=0; imsi<M->img_disp_h; imsi++) {
            M->img_data[(M->img_disp_y + imsi)* M->img_w + M->img_disp_x] = 0xff800000;
            M->img_data[(M->img_disp_y + imsi)* M->img_w + M->img_disp_x-1] = 0xff800000;

            M->img_data[(M->img_disp_y + imsi)* M->img_w + M->img_disp_x + M->img_disp_w] = 0xff800000;
            M->img_data[(M->img_disp_y + imsi)* M->img_w + M->img_disp_x + 1 + M->img_disp_w] = 0xff800000;
        }
        // Set orgMachine to the scaleFrame
        M->orgMachine->orgMachine = M->orgMachine = MachineCopy (M);
    }

    char winTitle[128];
    sprintf(winTitle, "MrMessy's %s [%s]", M->desc, M->name);
    image_context *img_ctx = image_init (M->img_w, M->img_h,
                                         M->orgMachine->img_w, M->orgMachine->img_h,
                                         winTitle, (M->orgMachine == &MachinePm3295));
    image_setmachine (img_ctx, M); // for image_manage to monitor knob's
    // ------------------------------------------------------------------------------
    // dbg
    image_showinfo(img_ctx);

    // ------------------------------------------------------------------------------
    // draw picture with interpolated scaling

    image_setsource  (img_ctx, M->img_data, M->img_w, M->img_h, 3); // such that image_manage can call image_display itself
    image_manage     (img_ctx); // image manage could call image_display()
    image_display    (img_ctx, M->img_data, M->img_w, M->img_h, 3);

    // ------------------------------------------------------------------------------
    // initialize audio system
    noudioJackInit(doConnect);

    // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Main processing loop

    int input_sz;
    int process_sz;
    int one_shot;

    one_shot   = true;

    while (one_shot || image_manage (img_ctx)) {

        // check if we need to resize after our event handler
        image_context_props imgChk = image_get_props(img_ctx);
        if (imgChk.winW != M->img_w || imgChk.winH != M->img_h) {

            // Resize the new machine by stretching the original machine to the new size
            // fprintf(stderr, "needs machine resize\n");

            pMachineDetails Morg = M->orgMachine;

            Hstretch = imgChk.winW - Morg->img_w;
            Vstretch = imgChk.winH - Morg->img_h;

            if (Hstretch < 0) Hstretch = 0;
            if (Vstretch < 0) Vstretch = 0;
            MachineFree (M);
            M = Morg;

            M = MachineHstretch(M, Hstretch);
            M = MachineVstretch(M, Vstretch);

            image_setmachine (img_ctx, M); // for image_manage to monitor knob's
            image_setsource  (img_ctx, M->img_data, M->img_w, M->img_h, 3);
            image_display    (img_ctx, M->img_data, M->img_w, M->img_h, 3);
        }


        if (M->orgMachine == &MachineHp8591a ||
            M->orgMachine != &MachinePm3295) {   // black machine

            input_sz   = 8192;
            process_sz = input_sz;

            noudioJackGetBuf(buf1, buf2, input_sz);
            process_sz = processBufs(input_sz);

            if ((G_clknbr % 1000) == 0) {
                fprintf(stderr, "fft_time: %10.8f [n=%d]\n", G_clksecs/(double)G_clknbr, G_clknbr);
            }
        }

        if (M->orgMachine == &MachinePm3295) {

            input_sz   = 2205;
            process_sz = input_sz;

            noudioJackGetBuf(buf1, buf2, input_sz);
            process_sz = processBufsScope(input_sz);
        }

        image_drawdata(img_ctx,
                       buf1, buf2, process_sz,
                       M->img_disp_x, M->img_disp_y,
                       M->img_disp_w, M->img_disp_h,
                       M->img_w,      M->img_h);

        one_shot = false;
    }

    // cleanup audio system
    noudioJackExit();

    // cleanup video system
    //free(videoFrame);
    image_exit(img_ctx);
    printf("done\n");
    //getchar();
    exit (0);
}

// =======================================================================

class Fft *theFft = 0;
complex   *fftIn  = 0;
complex   *fftOut = 0;

float t1[65536];
float t2[65536];
int   xtrans[65536];
int   xmult[65536];

double    powagc  = 0.001;

int processBufs(int sz)
{
    int     i;
    complex *C; // iterator
    float   *b1;
    float   *b2;

    int     sz_possible;
    int     sz_out;

    // find lowest power of two associated with sz
    int pw2=16;
    while((1<<pw2) > sz) pw2--;
    sz_possible=(1<<pw2);
    sz_out=sz_possible>>1;

    // see if we have an fft, and if it is of the correct size
    if (theFft) {
        if(theFft->FftSize != sz_possible) {
            delete   theFft; theFft = 0;
            delete[] fftIn;  fftIn  = 0;
            delete[] fftOut; fftOut = 0;
        }
    }
    // see if we must create an fft of the correct size
    if (!theFft) {
        theFft =  new Fft(pw2, 0, 8.0);
                     // (size = 2^pw2, Forwards: Dir == 0, window=1.0)
        fftIn  =  new complex[sz_possible];
        fftOut =  new complex[sz_possible];
        powagc =  0.001;

        // one time init, (for the I part)
        //for (i=0; i<sz_possible; i++) {
        //    fftIn[i].R = fftIn[i].I = 0;
        //}


        // 9 octaves ... out_size / 2^9 =0
#define SPAN (6)
#define FREQMAX (4.5)

        memset(xtrans, 0, sizeof(xtrans));
        memset(xmult,  0, sizeof(xmult));

        xtrans[0] = 0;
        xmult[0]  = 1;

        int xprv  = -1;
        for (i=1; i<sz_out; i++) {

            double x = (double)i/(double)(sz_out);
            double y = log(x)/log(2) + (SPAN-FREQMAX);

            if (y<-SPAN) y=-SPAN;

            // map -9...0 to 0...out_size
            y = (y+SPAN)*(double)sz_out/SPAN;
            if (y > sz_out)
                y=sz_out;

            xtrans[i]=y;

            if (xtrans[i] != xtrans[i-1]) {
                xmult[xtrans[i]] = 1;
            }
            else {
                xmult[xtrans[i]]++;
            }

            //fprintf(stderr, "xtrans[%4d] = %4d mult = %4d\n", i, xtrans[i], xmult[xtrans[i]]);

        }
    }
    // Check out if we have everything needed, if that didnt work, return 0
    if (!theFft || !fftIn || !fftOut) {
        return 0;
    }

    // prepare input
    C = fftIn;
    b1 = buf1;
    b2 = buf2;

    for (i=0; i<sz_possible; i++) {
        C->R = *b1++;
        C->I = *b2++;
        C++;
    }
    // transform
    clock_t sclk = clock();
    theFft->Transform(fftIn, fftOut);
    clock_t eclk = clock();
    G_clksecs += (double)(eclk - sclk)/(double)CLOCKS_PER_SEC;
    G_clknbr  += 1;



    // ============================================================
    // get output
    complex *Cpos = fftOut;
    complex *Cneg = fftOut;
    complex Lout;
    complex Rout;
    b1 = t1;
    b2 = t2;

    double pow;
    double powpeak;

    // untangle right and left, take power and register peakpower
    powpeak = 0;
    for (i=0; i<sz_out; i++) {

        // routing to split left and right data again
        Lout.R =  Cpos->R + Cneg->R;
        Lout.I =  Cpos->I - Cneg->I;
        Rout.R =  Cpos->I + Cneg->I;
        Rout.I = -Cpos->R + Cneg->R;

        pow = Lout.R*Lout.R + Lout.I*Lout.I;
        if (pow > powpeak) powpeak = pow;
        *b1++ = pow;

        pow = Rout.R*Rout.R + Rout.I*Rout.I;
        if (pow > powpeak) powpeak = pow;
        *b2++ = pow;

        Cpos++;
        Cneg--;
        if (!i) {Cneg = fftOut + (sz_possible - 1);}
    }

    // peakpower agc
    if (powpeak > powagc) {
        powagc = powagc + 0.0200*(powpeak-powagc);
    }
    else {
        powagc = powagc + 0.0005*(powpeak-powagc);
    }

    //fprintf(stderr, "powagc:%8f powpeak:%8f\n", powagc, powpeak);


    // scale output to peakpower agc
    #define LOGRANGE 50.0
    b1 = t1;
    b2 = t2;
    float tmp;
    for (i=0; i<sz_out; i++) {
        *b1 = *b1/powagc; //*b1/powagc;
        *b2 = *b2/powagc; //*b2/powagc;

        *b1 = 10.0*log10(*b1) + LOGRANGE;
        *b2 = 10.0*log10(*b2) + LOGRANGE;

        if (*b1 < 0) *b1 = 0;
        if (*b2 < 0) *b2 = 0;

        *b1 /= LOGRANGE;
        *b2 /= LOGRANGE;

        *b1 -= 0.5;
        *b2 -= 0.5;
        b1++;
        b2++;
    }

    // ====================================
    // scale to logarithmic frequency scale
    float *p1=t1;
    float *p2=t2;
    b1 = buf1;
    b2 = buf2;

    int xprv = 0;
    int xtrn = 0;

    *b1=0;
    *b2=0;

    for (i=0; i<sz_out; i++) {

        xtrn = xtrans[i];
        // fill b1,b2 from xprev to xtrans[i]
        for (; xprv<xtrn; xprv++) {
            *b1++ = *p1;
            *b2++ = *p2;
            *b1=0;
            *b2=0;
        }
        // new value
        *b1 += *p1/(float)xmult[xtrn];
        *b2 += *p2/(float)xmult[xtrn];

        p1++;
        p2++;
    }

    //fprintf(stderr, "buf:%8f t1:%8f\n", buf1[8191], t1[8191]);

    return sz_out;
}

// ------------------------------
int processBufsScope(int sz)
{
    int i;
    for (i=0; i<sz; i++) {
        buf1[i] *= 2.0;
        buf2[i] *= 2.0;

        buf1[i] -= 0.25;
        buf2[i] += 0.25;
    }
    return sz;
}
