// Header for machine details (scope, spectrum analyzer)
// (c)2014 Noudio
#ifndef __MACHINE_H__
#define __MACHINE_H__
#include <stdint.h> // for uint32_t

// ------------------------------------------------------------------------------------------------------------------
// forward declarations
typedef struct MachineDetails *pMachineDetails;
typedef struct KnobDetails    *pKnobDetails;
typedef int (*fncKnobPress)   (pKnobDetails This);
typedef int (*fncKnobRelease) (pKnobDetails This);
typedef int (*fncKnobDrag)    (pKnobDetails This, int x, int y);

struct DivElem {
    int refCoord;
    int divCoord;
};
typedef struct DivElem  DivElem;
typedef struct DivElem *pDivElem;

// ------------------------------------------------------------------------------------------------------------------
struct MachineDetails {
    char            name[32];         // name of the machine
    char            desc[32];         // description of the machine
    int             img_w;            // width of the 'original image'
    int             img_h;            // height of the 'original image'
    uint32_t       *img_data;         // 'original image' rgba pixels
    int             img_disp_x;       // display region topleft x
    int             img_disp_y;       // display region topleft y
    int             img_disp_w;       // display region width
    int             img_disp_h;       // display region height
    int             img_disp_nudge_x; // display region topleft x nudged for pixel replication
    int             img_disp_nudge_y; // display region topleft y nudged for pixel replication
    int             img_disp_nudge_w; // display region width nudged for pixel replication
    int             img_disp_nudge_h; // display region height nudged for pixel replication
    //int             img_division_x;   // x to divide image in left and right side
    //int             img_division_y;   // x to divide image in top and bottom side
    DivElem         img_division_x[6];
    DivElem         img_division_y[6];
    pMachineDetails orgMachine;       // link to original machine, to undo all mod's
    pKnobDetails    knobList;         // list of knob regions
};

// ------------------------------------------------------------------------------------------------------------------
struct KnobDetails {
    char name[32];           // name of the knob, control
    int  tlx;                // top left xy corner coordinates (or center for circle)
    int  tly;
    int  brx;                // bottom right xy corner coordinates (of the knob) (or radius for circle)
    int  bry;
    int  circle;             // if it is a circle shaped knob
    int  rhalf;              // wether this knob belongs to the right  half (true) or the left half (false)
    int  bhalf;              // wether this knob belongs to the bottom half (true) or the top  half (false)
    pMachineDetails machine; // machine to which this knob belongs to
    fncKnobPress    press;   // press function
    fncKnobRelease  release; // release function
    fncKnobDrag     drag;    // drag function
};

typedef struct KnobDetails KnobDetails;

// ------------------------------------------------------------------------------------------------------------------
extern MachineDetails MachineHp8591a;
extern MachineDetails MachinePm3295;
extern KnobDetails    KnobsHp8591a[];
extern KnobDetails    KnobsPm3295[];

// ------------------------------------------------------------------------------------------------------------------
// implemented functions
pMachineDetails MachineCopy (pMachineDetails M);
void            MachineFree (pMachineDetails M);


pMachineDetails MachineHstretch (pMachineDetails M, int Hstretch);
pMachineDetails MachineVstretch (pMachineDetails M, int Vstretch);

//void StretchHor (uint32_t *scaleFrame, int oWidth, int oHeight,
//                 uint32_t *videoFrame, int iWidth, int iHeight,
//                 int hDiv, int disp_y, int disp_h);

int knobDefaultPress   (pKnobDetails This);
int knobDefaultRelease (pKnobDetails This);
int knobDefaultDrag    (pKnobDetails This, int x, int y);

// ------------------------------------------------------------------------------------------------------------------
#endif // __MACHINE_H__
