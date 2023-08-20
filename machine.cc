// Machine details (scope, spectrum analyzer)
// (c)2014 Noudio

#include <stdlib.h> // for malloc, etc
#include <string.h> // memset, etc
#include <stdio.h>  // printfs

#include "machine.h"
#include "specselected.h"

// ------------------------------------------------------------------------------------------------------------------
MachineDetails MachineHp8591a = {
    "Hewlett Packard HP-8591A",
    "Spectrum Analyzer",
    651, 348,              // width and height
    (uint32_t *)gimp_image_hp8591a.pixel_data,
     92,  72,  199,  155,  // disp region
     92,  72,  199,  153,  // nudged disp region
    //180,  85,              // x,y division lines
    {
//        {348,    180},       // from y 0 to 348 division is at 180 (hstretch) (describes a vertical line)

        {230,    180},
        {252,    271},
        {298,    360},
        {348,    375}
    },
    {
        {359,     86},       // from x 0 to 359    y division is at  83 (vstretch) (describes a horizontal line)
        {439,     73},       // from  to 439       y division is at  73 (vstretch)
        {652,    115},
    },
    &MachineHp8591a,
    KnobsHp8591a
};
// ------------------------------------------------------------------------------------------------------------------
KnobDetails KnobsHp8591a[] = {
//    name           tlx  tly   brx  bry    circle, r-half  b-half,  machine           funcs p,r,d
    { "Display",      92,  72, 290,  224,   false,  false,  false,   &MachineHp8591a,   0, 0, 0},
    { "TLcorn",       15,   0,  97,   20,   false,  false,  false,   &MachineHp8591a,   0, 0, 0},
    { "BRcorn",      546, 333, 631,  346,   false,   true,   true,   &MachineHp8591a,   0, 0, 0},
    { "Power",        72, 279,  98,  291,   false,  false,   true,   &MachineHp8591a,   0, 0, 0},
    { "Preset",      453,  64, 472,   76,   false,   true,  false,   &MachineHp8591a,   0, 0, 0},
    { "Mode",        453,  89, 469,  100,   false,   true,  false,   &MachineHp8591a,   0, 0, 0},
    { "Volume",      212, 287,  13,    0,    true,   true,   true,   &MachineHp8591a,   0, 0, 0},
    { "Span",        379, 115, 423,  128,   false,   true,   true,   &MachineHp8591a,   0, 0, 0},
    { "Amp",         379, 143, 421,  157,   false,   true,   true,   &MachineHp8591a,   0, 0, 0},
    { "Freq",        379,  87, 422,  102,   false,   true,   true,   &MachineHp8591a,   0, 0, 0},
    { "Wheel",       401, 223,  25,    0,    true,   true,   true,   &MachineHp8591a,   0, 0, 0},
    { "Hold",        392, 172, 410,  184,   false,   true,   true,   &MachineHp8591a,   0, 0, 0},
    { "StepDown",    377, 279, 390,  296,   false,   true,   true,   &MachineHp8591a,   0, 0, 0},
    { "StepUp",      405, 278, 421,  296,   false,   true,   true,   &MachineHp8591a,   0, 0, 0},
    { "Softkey1",    330,  90, 343,   98,   false,   true,   true,   &MachineHp8591a,   0, 0, 0},
    { "Softkey2",    330, 112, 343,  122,   false,   true,   true,   &MachineHp8591a,   0, 0, 0},
    { "Softkey3",    330, 135, 343,  141,   false,   true,   true,   &MachineHp8591a,   0, 0, 0},
    { "Softkey4",    330, 158, 343,  167,   false,   true,   true,   &MachineHp8591a,   0, 0, 0},
    { "Softkey5",    329, 180, 343,  188,   false,   true,   true,   &MachineHp8591a,   0, 0, 0},
    { "Softkey6",    329, 203, 343,  212,   false,   true,   true,   &MachineHp8591a,   0, 0, 0},
    { "\0"}
};
// ------------------------------------------------------------------------------------------------------------------
MachineDetails MachinePm3295 = {
    "Philips PM-3295",
    "Oscilloscope",
    638, 318,              // width and height
    (uint32_t *)gimp_image_pm3295.pixel_data,
     47,  55,  195,  158,  // disp region
     47,  67,  195,  143,  // nudged disp region
    //166, 127,              // x,y division lines
    {
        {256,    166},       // from y 0 to 256 x division is at 166 (hstretch) (describes a vertical line)
        {318,    274}        // from y end  x division is at 274 (hstretch)
    },
    {
        {279,    127},       // from x 0    y division is at 127 (vstretch) (describes a horizontal line)
        {469,     19},       // from x 0    y division is at 127 (vstretch) (describes a horizontal line)
        {485,     14},
        {497,     19},
        {514,     14},
        {639,     19},       // from x 0    y division is at 127 (vstretch) (describes a horizontal line)
    },

    &MachinePm3295,
    KnobsPm3295
};
// ------------------------------------------------------------------------------------------------------------------
KnobDetails KnobsPm3295[] = {
//    name           tlx  tly   brx  bry    circle, r-half  b-half,  machine           funcs p,r,d
    { "Display",      47,  55, 241,  212,   false,  false,  false,   &MachinePm3295,    0, 0, 0},
    { "TLcorn",        6,   4, 125,   18,   false,  false,  false,   &MachinePm3295,    0, 0, 0},
    { "BRcorn",      618, 293, 634,  307,   false,   true,   true,   &MachinePm3295,    0, 0, 0},
    { "Power",        29, 278,  39,  287,   false,  false,   true,   &MachinePm3295,    0, 0, 0},
    { "Preset",      298,  34, 307,   44,   false,   true,  false,   &MachinePm3295,    0, 0, 0},
    { "WheelTB",     589, 143,  10,    0,    true,   true,   true,   &MachinePm3295,    0, 0, 0},
    { "WheelY1",     330, 146,  10,    0,    true,   true,   true,   &MachinePm3295,    0, 0, 0},
    { "WheelY1pos",  317,  68,   8,    0,    true,   true,  false,   &MachinePm3295,    0, 0, 0},
    { "EnableY1",    338,  34, 348,   45,   false,   true,  false,   &MachinePm3295,    0, 0, 0},
    { "ConnAcY1",    301, 212, 310,  223,   false,   true,   true,   &MachinePm3295,    0, 0, 0},
    { "ConnGndY1",   319, 212, 329,  223,   false,   true,   true,   &MachinePm3295,    0, 0, 0},
    { "ConnDcY1",    338, 211, 346,  224,   false,   true,   true,   &MachinePm3295,    0, 0, 0},
    { "WheelY2",     395, 146,  10,    0,    true,   true,   true,   &MachinePm3295,    0, 0, 0},
    { "WheelY2pos",  382,  68,   8,    0,    true,   true,  false,   &MachinePm3295,    0, 0, 0},
    { "EnableY2",    375,  34, 382,   45,   false,   true,  false,   &MachinePm3295,    0, 0, 0},
    { "ConnAcY2",    377, 211, 386,  222,   false,   true,   true,   &MachinePm3295,    0, 0, 0},
    { "ConnGndY2",   394, 211, 406,  222,   false,   true,   true,   &MachinePm3295,    0, 0, 0},
    { "ConnDcY2",    414, 211, 424,  221,   false,   true,   true,   &MachinePm3295,    0, 0, 0},
    { "\0"}
};

// ------------------------------------------------------------------------------------------------------------------
// Deep copy of source machine
pMachineDetails MachineCopy (pMachineDetails M)
{
    pMachineDetails R;

    // protect from illegal input
    R = 0;
    if (!(M && M->img_data && M->knobList)) {
        fprintf(stderr, "MachineCopy: illegal input.\n");
        return R;
    }

    // get mem
    R = (pMachineDetails) malloc(sizeof(MachineDetails));
    if (!R) {
        fprintf(stderr, "MachineCopy: out of mem.\n");
        return R;
    }

    // copy basic fields
    memset(R, 0, sizeof(MachineDetails));
    memcpy(R, M, sizeof(MachineDetails));

    // zero out dynamic allocated memory
    R->img_data = 0;
    R->knobList = 0;

    // image data with new malloced copy
    R->img_data = (uint32_t *) malloc (M->img_w * M->img_h * sizeof(uint32_t));
    if (!R->img_data) {
        MachineFree(R);
        R = 0;
        fprintf(stderr, "MachineCopy: out of mem.\n");
        return R;
    }
    // Copy over image data
    memcpy (R->img_data, M->img_data, M->img_w * M->img_h * sizeof(uint32_t));

    // find out if we should do a rgba conversion, (of the gimp image)
    if ((M->img_data == (uint32_t *)gimp_image_hp8591a.pixel_data) ||
        (M->img_data == (uint32_t *)gimp_image_pm3295.pixel_data)) {

        // original gimp images, need conversion: R and B are interchanged

        for (int i=0; i < M->img_w * M->img_h; i++) {

            uint32_t h = M->img_data[i];

            R->img_data[i] =  (h & 0xff000000) | ((h & 0x00ff0000) >> 16) |
                              (h & 0x0000ff00) | ((h & 0x000000ff) << 16);
        }
    }

    // knobList, find out how many elements there are
    int nrKnobs = 0;
    pKnobDetails P;
    for (P = M->knobList; P && P->name && P->name[0]; P++)
        nrKnobs++;

    // new knoblist data
    R->knobList = (pKnobDetails) malloc( (nrKnobs+1) * sizeof(KnobDetails));
    if (!R->knobList) {
        MachineFree(R);
        R = 0;
        fprintf(stderr, "MachineCopy: out of mem.\n");
        return R;
    }

    // copy knobList data
    memcpy(R->knobList, M->knobList, (nrKnobs+1) * sizeof(KnobDetails));
    // force zero sentinel
    memset(&R->knobList[nrKnobs], 0, sizeof(KnobDetails));


    // set default functions of the knobs
    //pKnobDetails P;
    for (P = R->knobList; P && P->name && P->name[0]; P++) {
        P->press   = knobDefaultPress;
        P->release = knobDefaultRelease;
        P->drag    = knobDefaultDrag;
    }

    // done
    return R;
}

// ------------------------------------------------------------------------------------------------------------------
// Free, but not static allocated memory

void MachineFree (pMachineDetails M)
{
    if (!M)
        return;

    if (M->img_data &&
        (M->img_data != (uint32_t *)gimp_image_hp8591a.pixel_data) &&
        (M->img_data != (uint32_t *)gimp_image_pm3295.pixel_data)) {

        free(M->img_data);
        M->img_data = 0;
    }

    if (M->knobList &&
        (M->knobList != KnobsHp8591a) &&
        (M->knobList != KnobsPm3295)) {

        free(M->knobList);
        M->knobList = 0;
    }

    if ((M != &MachineHp8591a) &&
        (M != &MachinePm3295)) {
        free(M);
        M = 0;
    }
}

// =====================================================================================
#define NA 3
uint32_t noisePixel(uint32_t p)
{
    uint32_t r, rp;
    int32_t  r2, cl, i;

    rp = p & 0xff000000; // retain alfa

    for (i=0; i<= 16; i+= 8) {
        // random
        r  = lrand48();
        r2 = (r % NA) +
        ((r/(NA))%NA) +
        ((r/(NA*NA))%NA) +
        ((r/(NA*NA*NA))%NA) +
        ((r/(NA*NA*NA*NA))%NA) - 5*(NA-1)/2;

        // color i
        cl = ((p>>i) & 0xff) + (r2>>1);
        if (cl > 255) cl = 255;
        if (cl <   0) cl = 0;

        // add
        rp = rp | (cl<<i);
    }
    return rp;
}

// =====================================================================================

uint32_t *stretchH (uint32_t *videoFrame, int iWidth, int iHeight,
                    int Hstretch,
                    pDivElem hDivPtr,
                    int disp_y, int disp_h)
{
    int x, y, idxO, idxI;

    int oWidth  = iWidth + Hstretch;
    int oHeight = iHeight;

    uint32_t *scaleFrame = (uint32_t*) malloc (oWidth*oHeight*sizeof(uint32_t));
    if (!scaleFrame) {
        fprintf(stderr, "stretchH:: out of mem\n");
        return scaleFrame;
    }

    // ----------------------------------------------------
    memset(scaleFrame, 0, oWidth*oHeight*sizeof(uint32_t));


    for (y=0; y<iHeight; y++) {

        if (y >= hDivPtr->refCoord) {
            hDivPtr++;
        }

        int hDiv = hDivPtr->divCoord;
        int hEnd = oWidth - (iWidth - hDiv);

        // ---------------------------------------
        // copy first part

        for (x=0; x<hDiv; x++) {

            idxI = y*iWidth + x;
            idxO = y*oWidth + x;
            scaleFrame[idxO] = videoFrame[idxI];
        }
        // ---------------------------------------
        // stretch centerpart

        for (; x<hEnd; x++) {

            int yr = y;

            if ((y>=disp_y) && (y < disp_y+disp_h)) {

                yr = (lrand48() % 15) - 7 + y;
                if (yr < disp_y) yr = disp_y;
                if (yr >= disp_y+disp_h) yr = disp_y+disp_h-1;
            }

            idxI = yr*iWidth + hDiv-1;
            idxO = y*oWidth + x;

            scaleFrame[idxO] = videoFrame[idxI]; // noisePixel(videoFrame[idxI]);
        }

        // ---------------------------------------
        // copy last part
        for (; x<oWidth; x++) {

            idxI = y*iWidth + x - hEnd + hDiv;
            idxO = y*oWidth + x;

            scaleFrame[idxO] = videoFrame[idxI];
        }

    } // for y
    // ---------------------------------------
    // Return the stretched scaleFrame
    return scaleFrame;
}

// =====================================================================================

pMachineDetails MachineHstretch (pMachineDetails M, int Hstretch)
{
    M = MachineCopy(M);
    if (!M) {
        fprintf(stderr, "MachineHstretch:: out of mem\n");
        return M;
    }

    uint32_t *new_data = stretchH (M->img_data, M->img_w, M->img_h,
                                   Hstretch,
                                   M->img_division_x,
                                   M->img_disp_nudge_y, // nudged display y region
                                   M->img_disp_nudge_h);
    if (!new_data) {
        MachineFree(M);
        M = 0;
        fprintf(stderr, "MachineHstretch:: out of mem\n");
        return M;
    }

    // replace image
    free(M->img_data);
    M->img_data   = new_data;
    // register new width's
    M->img_disp_w = M->img_disp_w + Hstretch;
    M->img_w      = M->img_w      + Hstretch;

    // change places of the knobs'
    pKnobDetails P;
    for (P = M->knobList; P && P->name && P->name[0]; P++) {

        // what is P's x, and find division based on x coord
        pDivElem D;
        int      divX;

        D = M->img_division_x;

        while (P->tly >= D->refCoord) D++;
        divX = D->divCoord;

        if (P->tlx > divX) P->tlx = P->tlx + Hstretch;

        while (P->bry >= D->refCoord) D++;
        divX = D->divCoord;

        if (P->brx > divX && !P->circle) P->brx = P->brx + Hstretch;
    }

    // ------------------------------------------------------------------
    // the divsions of the other direction must be stretched too
    for (int i=0; i<sizeof(M->img_division_y)/sizeof(DivElem); i++) {

        pDivElem Y = &M->img_division_y[i];

        // what is P's x, and find division based on x coord
        pDivElem D;
        int      divX;

        D = M->img_division_x;

        while (Y->divCoord >= D->refCoord) D++;
        divX = D->divCoord;

        if (Y->refCoord > divX) Y->refCoord += Hstretch;
    }

    return M;
}





// =====================================================================================

uint32_t *stretchV (uint32_t *videoFrame, int iWidth, int iHeight,
                    int Vstretch,
                    pDivElem vDivPtr,
                    int disp_x, int disp_w)
{
    int x, y, idxO, idxI;

    int oWidth  = iWidth;
    int oHeight = iHeight + Vstretch;

    uint32_t *scaleFrame = (uint32_t*) malloc (oWidth*oHeight*sizeof(uint32_t));
    if (!scaleFrame) {
        fprintf(stderr, "stretchV:: out of mem\n");
        return scaleFrame;
    }

    // ----------------------------------------------------
    memset(scaleFrame, 0, oWidth*oHeight*sizeof(uint32_t));

    for (x=0; x<iWidth; x++) {

        if (x >= vDivPtr->refCoord) {
            vDivPtr++;
        }

        int vDiv = vDivPtr->divCoord;
        int vEnd = oHeight - (iHeight - vDiv);

        // ---------------------------------------
        // copy first part

        for (y=0; y<vDiv; y++) {

            idxI = y*iWidth + x;
            idxO = y*oWidth + x;
            scaleFrame[idxO] = videoFrame[idxI];
        }
        // ---------------------------------------
        // stretch centerpart
        for (; y<vEnd; y++) {

            int xr = x;

            if ((x>=disp_x) && (x < disp_x+disp_w)) {

                xr = (lrand48() % 15) - 7 + x;
                if (xr < disp_x) xr = disp_x;
                if (xr >= disp_x+disp_w) xr = disp_x+disp_w-1;
            }

            idxI = (vDiv-1)*iWidth + xr;
            idxO = y*oWidth + x;

            scaleFrame[idxO] = videoFrame[idxI]; // noisePixel(videoFrame[idxI]);
        }
        // ---------------------------------------
        // copy last part
        for (; y<oHeight; y++) {

            //idxI = y*iWidth + x - hEnd + hDiv;
            idxI = (y - vEnd + vDiv)*iWidth + x;
            idxO = y*oWidth + x;

            scaleFrame[idxO] = videoFrame[idxI];
        }
    } // y
    // ---------------------------------------
    // Return the stretched scaleFrame
    return scaleFrame;
}

// =====================================================================================

pMachineDetails MachineVstretch (pMachineDetails M, int Vstretch)
{
    M = MachineCopy(M);
    if (!M) {
        fprintf(stderr, "MachineVstretch:: out of mem\n");
        return M;
    }

    uint32_t *new_data = stretchV (M->img_data, M->img_w, M->img_h,
                                   Vstretch,
                                   M->img_division_y,
                                   M->img_disp_nudge_x, // nudged display y region
                                   M->img_disp_nudge_w);
    if (!new_data) {
        MachineFree(M);
        M = 0;
        fprintf(stderr, "MachineVstretch:: out of mem\n");
        return M;
    }

    // replace image
    free(M->img_data);
    M->img_data   = new_data;
    // register new width's
    M->img_disp_h = M->img_disp_h + Vstretch;
    M->img_h      = M->img_h      + Vstretch;

    // ------------------------------------------------------------------
    // change places of the knobs'
    pKnobDetails P;
    for (P = M->knobList; P && P->name && P->name[0]; P++) {

        // what is P's x, and find division based on x coord
        pDivElem D;
        int      divY;

        D = M->img_division_y;

        while (P->tlx >= D->refCoord) D++;
        divY = D->divCoord;

        if (P->tly > divY) P->tly = P->tly + Vstretch;

        while (P->brx >= D->refCoord) D++;
        divY = D->divCoord;

        if (P->bry > divY && !P->circle) P->bry = P->bry + Vstretch;
    }

    // ------------------------------------------------------------------
    // the divsions of the other direction might be stretched too
    for (int i=0; i<sizeof(M->img_division_x)/sizeof(DivElem); i++) {

        pDivElem X = &M->img_division_x[i];

        // what is P's x, and find division based on x coord
        pDivElem D;
        int      divY;

        D = M->img_division_y;

        while (X->divCoord >= D->refCoord) D++;
        divY = D->divCoord;

        if (X->refCoord > divY) X->refCoord += Vstretch;
    }


    return M;
}

// =====================================================================================
int knobDefaultPress   (pKnobDetails This)
{
    fprintf(stderr, "%s(%s) pressed.\n", This->name, This->machine->name);
    return 0;
}
// =====================================================================================
int knobDefaultRelease (pKnobDetails This)
{
    fprintf(stderr, "%s(%s) released.\n", This->name, This->machine->name);
    return 0;
}
// =====================================================================================
int knobDefaultDrag    (pKnobDetails This, int x, int y)
{
    int cx, cy; // center of knob;

    cx = This->tlx;
    cy = This->tly;

    if (!This->circle) {
        cx = (This->tlx + This->brx)/2;
        cy = (This->tly + This->bry)/2;
    }

    fprintf(stderr, "%s(%s) dragged.[%3d,%3d]\n", This->name, This->machine->name, x-cx, y-cy);
    return 0;
}
