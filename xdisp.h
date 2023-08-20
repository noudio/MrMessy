// Header for Xviewer routines
// (c)2013 Noudio
#ifndef __XDISP_H__
#define __XDISP_H__
#include <stdint.h>
#include "machine.h"

// ----------------------------------------------------------
typedef struct image_context image_context;
// -------------------------------------------------------------
// internal protos
image_context *image_init       (                        int width, int height,  int minw, int minh, const char *title, int is_scope);
int            image_exit       (image_context *img_ctx);
int            image_manage     (image_context *img_ctx); // handles all events, returns false if exit requested

void           image_setmachine (image_context *img_ctx, pMachineDetails M);

void           image_setsource  (image_context *img_ctx, uint32_t *rgb32Frame, int width, int height, int Scaled);
void           image_display    (image_context *img_ctx, uint32_t *rgb32Frame, int width, int height, int Scaled);
void           image_showinfo   (image_context *img_ctx);

void           image_drawdata   (image_context *img_ctx,
                                 float *buf1,
                                 float *buf2,
                                 int   buf_sz,
                                 int   disp_x, int disp_y,
                                 int   disp_w, int disp_h,
                                 int   mach_w, int mach_h);


// ---------------------------------------------------------
// things that are avaiable for get
typedef struct image_context_props
{
    // for displaying
    int                  winW;
    int                  winH;
} image_context_props;

image_context_props image_get_props(image_context *img_ctx);

#endif
