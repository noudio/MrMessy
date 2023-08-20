/*This sample program was made by:
   noudio...
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>             /* getopt_long() */
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/shape.h>

#include "pixfilt.h"
#include "machine.h"
#include "xdisp.h"

#define IMG_MINWIDTH  120
#define IMG_MINHEIGHT 90
#define IMG_MAXWIDTH  2880
#define IMG_MAXHEIGHT 1650

// ----------------------------------------------------------

typedef struct image_context
{
    Display              *display;
    GC                   gc;
    Visual               *visual;
    int                  depth;
    Bool                 isShared; // MITSHM
    XImage               *xImage;
    Pixmap               sharedPixmap; // None (0L) if unassigned
    XShmSegmentInfo      shmInfo;

    XEvent               event;
    Window               window;
    int                  screenNumber;
    Atom                 atomWMDeleteWindow;
    Screen               *screen;
    Colormap             colormap;
    XWindowAttributes    windowAttributes;

    // for displaying
    int                  winW;
    int                  winH;

    int                  minW;
    int                  minH;

    int                  scalerSz;
    uint32_t             *scalerData;

    int                  scl2Sz;
    uint32_t             *scl2Data;

    int                   pixFiltType;
    pixFilt_t            *pixFilt;


    // void           image_setsource (image_context *img_ctx, uint32_t *rgb32Frame, int width, int height, int Scaled);
    uint32_t             *srcFrame;
    int                   srcWidth;
    int                   srcHeight;
    int                   srcScaleOption;

    pMachineDetails       machine;
    pKnobDetails          machine_knob_pressed;

} image_context;


char *image_data_cache;
int   image_data_cache_sz;

int   G_is_scope         = false;
int   G_in_image_manage  = false;
int   G_decoration_state = false;


// -------------------------------------------------------------
// internal protos
image_context *
     image_init         (                        int width, int height, int minw,   int minh,   const char *title, int is_scope);
int  image_exit         (image_context *img_ctx);
int  image_create       (image_context *img_ctx, int width, int height, Bool wantShared, Bool wantSharedPixmap);
int  image_destroy      (image_context *img_ctx);
int  image_put          (image_context *img_ctx, int srcX, int srcY,    int dstX,   int dstY,   int width, int height);
void image_display      (image_context *img_ctx, uint32_t *rgb32Frame,  int width,  int height, int Scaled);
void image_scale        (image_context *img_ctx, uint32_t *rgb32Out,    int oWidth, int oHeight,
                                                 uint32_t *rgb32In,     int iWidth, int iHeight);
void image_scale_filtered (image_context *img_ctx,
                           uint32_t *rgb32Out, int oWidth, int oHeight,
                           uint32_t *rgb32In,  int iWidth, int iHeight);

void image_showinfo     (image_context *img_ctx);
int  image_manage       (image_context *img_ctx); // handles all events, returns false for 'exit' request

void image_decoration   (image_context *img_ctx, int decoration); // turn on/off window dressing (he routine cannot be used when window is mapped
void image_setmachine   (image_context *img_ctx, pMachineDetails M); // for knob lists
image_context_props     image_get_extents(image_context *img_ctx);

// ----------------------------------------------------------
image_context *image_init (int width, int height, int minw, int minh, const char *title, int is_scope)
{
    image_context *img_ctx;
    int Dw, Dh;

    image_data_cache    = (char *)0;
    image_data_cache_sz = 0;
    G_in_image_manage   = false;
    G_is_scope          = is_scope != 0;
    G_decoration_state  = true;

    img_ctx = (image_context *)malloc(sizeof(image_context));
    if (!img_ctx) {
        perror("malloc");
        return NULL;
    }
    memset(img_ctx, 0, sizeof(image_context));

    // configure window and display
    // try to open the display
    if ((img_ctx->display = XOpenDisplay(NULL)) == NULL)
    {
        perror("XOpenDisplay()");
        return NULL;
    }

    // get display size
    {
        Window wid = 0;
        XWindowAttributes xwAttr;

        wid = DefaultRootWindow( img_ctx->display );
        if ( 0 > wid ) {
            fprintf(stderr, "Failed to obtain the root windows Id "
            "of the default screen of given display.\n");
            return NULL;
        }
        Status ret = XGetWindowAttributes( img_ctx->display, wid, &xwAttr );
        Dw = xwAttr.width;
        Dh = xwAttr.height;

        fprintf(stderr, "display size: %d x %d\n", Dw, Dh);
        fflush(stderr);
    }

    //get default display number
    img_ctx->screenNumber = DefaultScreen(img_ctx->display);
    //associate screen with the default display
    img_ctx->screen = XScreenOfDisplay(img_ctx->display, img_ctx->screenNumber);

//    int yoff = 0;
//    G_is_scope = false;
//    if (!strcmp(title, "MrMessy's Oscilloscope")) {
//        yoff = 348;
//        G_is_scope = true;
//    }
    int yoff = is_scope ? 348 : 0;



    //====================================
    //size checks

    if (minw < IMG_MINWIDTH)  minw = IMG_MINWIDTH;
    if (minh < IMG_MINHEIGHT) minh = IMG_MINHEIGHT;

    if (width < minw) {
        width = minw;
        fprintf(stderr, "image_init::width < minwidth\n");
    }
    if (width > IMG_MAXWIDTH) {
        width = IMG_MAXWIDTH;
        fprintf(stderr, "image_init::width > maxwidth\n");
    }
    if (height < minh) {
        height = minh;
        fprintf(stderr, "image_init::height < minheight\n");
    }
    if (height > IMG_MAXHEIGHT) {
        height = IMG_MAXHEIGHT;
        fprintf(stderr, "image_init::height > maxheight\n");
    }
    img_ctx->minW = minw;
    img_ctx->minH = minh;

    //============================================
    //create the window
    img_ctx->window = XCreateSimpleWindow(
        img_ctx->display,
        RootWindowOfScreen(img_ctx->screen),
        Dw-width, // x
        yoff, // y
        width, // width
        height, // height
        0,                          // border width
        BlackPixelOfScreen(img_ctx->screen), // border
        BlackPixelOfScreen(img_ctx->screen)  // background
    );

    //Set image decoration
    image_decoration(img_ctx, G_decoration_state);

    {
        // set size hints
        XSizeHints S;

        memset(&S, 0, sizeof(S));


        //img_ctx->minW

        S.base_width  = width;
        S.min_width   = img_ctx->minW;
        S.max_width   = IMG_MAXWIDTH;

        S.base_height  = height;
        S.min_height   = img_ctx->minH;
        S.max_height   = IMG_MAXHEIGHT;

        S.min_aspect.x = width;
        S.max_aspect.x = width;
        S.min_aspect.y = height;
        S.max_aspect.y = height;

        S.flags = PBaseSize | PMinSize | PMaxSize /*| PAspect */;
        XSetWMNormalHints(img_ctx->display, img_ctx->window, &S);
    }

/*
     {
        // subscribe to events
        XSetWindowAttributes A;
        memset(&A, 0, sizeof(A));
        A.event_mask = ExposureMask | ResizeRedirectMask;
        XChangeWindowAttributes(img_ctx->display, img_ctx->window, CWEventMask, &A);
    }
    //oh well...
*/
    {
        long event_mask =  StructureNotifyMask |
                           ExposureMask        |
                           PropertyChangeMask  |
                           EnterWindowMask     |
                           LeaveWindowMask     |
                           KeyPressMask        |
                           KeyReleaseMask      |
                           ButtonPressMask     |
                           ButtonReleaseMask   |
                           PointerMotionMask | ButtonMotionMask | Button1MotionMask |
                           KeymapStateMask;

        XSelectInput(img_ctx->display, img_ctx->window, event_mask);
    }


    img_ctx->xImage = NULL;//xImage is not allocated yet

    if (image_create(img_ctx, IMG_MAXWIDTH, IMG_MAXHEIGHT, True, False) < 0)
    {
        perror("image_create()");
        return NULL;
    }

    XMapRaised(img_ctx->display, img_ctx->window);
    XStoreName(img_ctx->display, img_ctx->window, title);
    XGetWindowAttributes(img_ctx->display, img_ctx->window, &(img_ctx->windowAttributes));
    img_ctx->winW  = img_ctx->windowAttributes.width;
    img_ctx->winH  = img_ctx->windowAttributes.height;

    img_ctx->scalerSz = 0;
    img_ctx->scalerData = (uint32_t *)0;

    img_ctx->scl2Sz = 0;
    img_ctx->scl2Data = (uint32_t *)0;

    img_ctx->pixFiltType = 0;
    img_ctx->pixFilt = (pixFilt_t *)0;

    // Wait for the first (exposure) event, before we draw anything, but leave it on the event queue
    {
        XEvent E;
        do {
            XPeekEvent(img_ctx->display, &E);
            if (E.type != Expose) {
                // if it is not an expose, then handle it
                XNextEvent(img_ctx->display, &E);
            }
        } while (E.type != Expose);
    }
    //test_pixfilt();
    return img_ctx;
}
// ----------------------------------------------------------
int image_exit (image_context *img_ctx)
{
    image_destroy(img_ctx);

    if (img_ctx->display != NULL)
    {
        if (img_ctx->window != None) {
           XDestroyWindow(img_ctx->display, img_ctx->window);
           img_ctx->window = None;
        }
        XCloseDisplay(img_ctx->display);
        img_ctx->display = NULL;
    }

    if (img_ctx->scalerData) {
        free(img_ctx->scalerData);
        img_ctx->scalerData = (uint32_t *)0;
        img_ctx->scalerSz = 0;
    }
    if (img_ctx->scl2Data) {
        free(img_ctx->scl2Data);
        img_ctx->scl2Data = (uint32_t *)0;
        img_ctx->scl2Sz = 0;
    }
    if (img_ctx->pixFilt)  {
    	destroyPixFilt(img_ctx->pixFilt);
    	img_ctx->pixFilt = (pixFilt_t *)0;
    	img_ctx->pixFiltType = 0;
    }

    int                   pixFiltType;
    pixFilt_t            *pixFilt;


    memset(img_ctx, 0, sizeof(image_context));
    free(img_ctx);

    if (image_data_cache && image_data_cache_sz) {
       free(image_data_cache);
       image_data_cache    = (char *)0;
       image_data_cache_sz = 0;
    }

    if (img_ctx->srcFrame) {
        free(img_ctx->srcFrame);
        img_ctx->srcFrame = 0;
    }
    img_ctx->srcWidth  = 0;
    img_ctx->srcHeight = 0;
    img_ctx->srcScaleOption = 0;

    return 0;
}
// ----------------------------------------------------------
int image_create(image_context * img_ctx, int width, int height, Bool wantShared, Bool wantSharedPixmap)
{
	int                majorVersion;
	int                minorVersion;
	Bool               sharedPixmapsSupported;
	XGCValues          gcValues;
	ulong              gcValuesMask;
	XWindowAttributes  windowAttributes;

    // creates: gc, xImage
	if (img_ctx->xImage != NULL)
	{
		image_destroy(img_ctx);
	}

	gcValues.function = GXcopy;
	gcValuesMask = GCFunction;
	img_ctx->gc = XCreateGC(img_ctx->display, img_ctx->window, gcValuesMask, &gcValues);
/*
    XSetClipOrigin(display, gc, clip_x_origin, clip_y_origin)
      Display *display;
      GC gc;
      int clip_x_origin, clip_y_origin;
*/
    XSetClipMask(img_ctx->display, img_ctx->gc, None);


	XGetWindowAttributes(img_ctx->display, img_ctx->window, &windowAttributes);

	img_ctx->visual = windowAttributes.visual;
	img_ctx->depth = windowAttributes.depth;

	if (wantShared && XShmQueryExtension(img_ctx->display))
	{
		img_ctx->isShared = 1;
	} else
	{
		img_ctx->isShared = 0;
	}

	errno = 0;
	img_ctx->xImage = NULL;
	img_ctx->sharedPixmap = None;
	if (img_ctx->isShared)
	{
		img_ctx->shmInfo.shmid = -1;
		img_ctx->shmInfo.shmaddr = NULL;
		if ((img_ctx->xImage = XShmCreateImage(img_ctx->display, img_ctx->visual,
			img_ctx->depth, ZPixmap, NULL, &(img_ctx->shmInfo), width, height)) == NULL)
		{
			return -1;
		}
		if ((img_ctx->shmInfo.shmid = shmget(IPC_PRIVATE,
			img_ctx->xImage->bytes_per_line * img_ctx->xImage->height,
			IPC_CREAT | 0777)) < 0)
		{ // Create segment
			return -1;
		}
		if ((img_ctx->shmInfo.shmaddr = (char *) shmat(img_ctx->shmInfo.shmid, 0, 0)) == (void *)-1)
		{  // We attach
			img_ctx->shmInfo.shmaddr = NULL;
			return -1;
		}
		img_ctx->xImage->data = img_ctx->shmInfo.shmaddr;
		img_ctx->shmInfo.readOnly = False;
		if (!XShmAttach(img_ctx->display, &(img_ctx->shmInfo)))
		{ // X attaches
			return -1;
		}
		if (wantSharedPixmap && (XShmPixmapFormat(img_ctx->display) == ZPixmap))
		{
			if ((img_ctx->sharedPixmap = XShmCreatePixmap(img_ctx->display, img_ctx->window,
				img_ctx->shmInfo.shmaddr, &(img_ctx->shmInfo), width, height, img_ctx->depth))
				== None)
			{
				return -1;
			}
		}
	} else
	{
		if ((img_ctx->xImage = XCreateImage(img_ctx->display, img_ctx->visual,
			img_ctx->depth, ZPixmap, 0, NULL, width, height, 16, 0)) == NULL)
		{
			return -1;
		}

		img_ctx->xImage->data =
			(char *) malloc(img_ctx->xImage->bytes_per_line * img_ctx->xImage->height);

		if (img_ctx->xImage->data == NULL)
		{
			return -1;
		}
	}

	memset(img_ctx->xImage->data, 0, img_ctx->xImage->bytes_per_line * img_ctx->xImage->height);
	return 0;
}
// ----------------------------------------------------------
int image_destroy(image_context *img_ctx)
{
    if (img_ctx->xImage != NULL) {

        if (img_ctx->isShared)
        {
            if (img_ctx->shmInfo.shmid >= 0)
            {
                XShmDetach(img_ctx->display, &(img_ctx->shmInfo)); // X detaches
                shmdt(img_ctx->shmInfo.shmaddr); // We detach
                img_ctx->shmInfo.shmaddr = NULL;
                shmctl(img_ctx->shmInfo.shmid, IPC_RMID, 0); // Destroy segment
                img_ctx->shmInfo.shmid = -1;
            }
        } else
        {
            if (img_ctx->xImage->data != NULL)
            {
                free(img_ctx->xImage->data);
            }
        }
        img_ctx->xImage->data = NULL;

        XDestroyImage(img_ctx->xImage);
        img_ctx->xImage = NULL;
    }

    if (img_ctx->sharedPixmap != None)
    {
        XFreePixmap(img_ctx->display, img_ctx->sharedPixmap);
        img_ctx->sharedPixmap = None;
    }

    if (img_ctx->gc != NULL) {
        XFreeGC(img_ctx->display, img_ctx->gc);
        img_ctx->gc = NULL;
    }
    return 0;
}

// ----------------------------------------------------------

void image_decoration(image_context *img_ctx, int decoration)
{
    // set splash screen decoration
    //Atom type = XInternAtom(img_ctx->display, "_NET_WM_WINDOW_TYPE", False);
    //Atom value = XInternAtom(img_ctx->display, "_NET_WM_WINDOW_TYPE_SPLASH", False); // promising
    //Atom value = XInternAtom(img_ctx->display, "_NET_WM_WINDOW_TYPE_DOCK", False); // promising
    //Atom value = XInternAtom(img_ctx->display, "_NET_WM_WINDOW_TYPE_MENU", False); // decoration
    //Atom value = XInternAtom(img_ctx->display, "_NET_WM_WINDOW_TYPE_TOOLBAR", False); // most promising
    //Atom value = XInternAtom(img_ctx->display, "_NET_WM_WINDOW_TYPE_UTILITY", False); // decoration
    //Atom value = XInternAtom(img_ctx->display, "_NET_WM_WINDOW_TYPE_DESKTOP", False); // promising, background



    Atom type;
    Atom value;

    type = XInternAtom(img_ctx->display, "_NET_WM_WINDOW_TYPE", False);

    if (!decoration) {
        // splash has no decoration but is not resieable.. touch the top left corner
        // becoming decorated needs the window to be unmapped and mapped
        value = XInternAtom(img_ctx->display, "_NET_WM_WINDOW_TYPE_SPLASH", False); // most promising
    }
    else {
        // --> dec. resizable  dialog
        value = XInternAtom(img_ctx->display, "_NET_WM_WINDOW_TYPE_NORMAL",  False);
    }
    XChangeProperty(img_ctx->display, img_ctx->window, type, XA_ATOM, 32, PropModeReplace, (unsigned char *)(&value), 1);
    G_decoration_state = decoration;

}

// ----------------------------------------------------------

// void           image_setsource (im
//uint32_t             *srcFrame;
//int                   srcWidth;
//int                   srcHeight;
//int                   srcScaleOption;
void image_setsource (image_context *img_ctx,
                      uint32_t *rgb32Frame,
                      int width, int height,
                      int Scaled)
{
    if (!img_ctx)
        return;

    img_ctx->srcWidth  = 0;
    img_ctx->srcHeight = 0;
    img_ctx->srcScaleOption = 0;
    img_ctx->srcFrame  = (uint32_t *)malloc(width*height*sizeof(uint32_t));
    if (!img_ctx->srcFrame) {
        fprintf(stderr, "image_setsource: no mem\n");
        return;
    }
    memcpy(img_ctx->srcFrame, rgb32Frame, width*height*sizeof(uint32_t));
    img_ctx->srcWidth  = width;
    img_ctx->srcHeight = height;
    img_ctx->srcScaleOption = Scaled;
}
// ----------------------------------------------------------
void image_setmachine   (image_context *img_ctx, pMachineDetails M)
{
    // --------------------------------
    // if knob still pressed, release first

    pKnobDetails K;
    K = img_ctx->machine_knob_pressed;
    if (K && K->release) {
        K->release(K);
    }
    img_ctx->machine_knob_pressed = 0;

    // ---------------------------------
    // for knob lists
    img_ctx->machine = M;
}
// ----------------------------------------------------------
int image_put (image_context *img_ctx, int srcX, int srcY, int dstX, int dstY,
				int width, int height) {

	if (img_ctx->xImage == NULL) return (-1);

	if (width < 0) width   = img_ctx->xImage->width;
	if (height < 0) height = img_ctx->xImage->height;

	if (img_ctx->isShared)
	{
        //printf("shared\n");
		XShmPutImage(img_ctx->display, img_ctx->window, img_ctx->gc, img_ctx->xImage,
						srcX, srcY, dstX, dstY, width, height, False /*True*/);
	} else
	{
        //printf("not shared\n");
		XPutImage(img_ctx->display, img_ctx->window, img_ctx->gc, img_ctx->xImage,
					srcX, srcY, dstX, dstY, width, height);
	}

	//XCopyArea(img_ctx->display, img_ctx->sharedPixmap, img_ctx->window, 0,0, width, height, 0,0);
	return 0;
}

// ----------------------------------------------------------
//read the data from memory, converts that data to RGB, and call Put (shows the picture)
void image_display (image_context *img_ctx, uint32_t *rgb32Frame,  int width, int height, int Scaled)
{
    // private local subs
    void      memcpy_32 (uint8_t *iDst, uint32_t *iSrc, int iSrcLineSz);
    void      memcpy_24 (uint8_t *iDst, uint32_t *iSrc, int iSrcLineSz);
    void      memcpy_16 (uint8_t *iDst, uint32_t *iSrc, int iSrcLineSz);

    XImage   *xImage1 = img_ctx->xImage;
    uint32_t *iSrc;
    uint8_t  *iDst;
    int      iSrcLineSz;
    int      iDstLineSz;
    int      y;

    // hanfle events get windowsize etc
    // NOTE: this shouldnt be neccessary
    // image_manage(img_ctx);

    //printf("bytes_per_line:%d\n", xImage1->bytes_per_line);
    //printf("bytes_per_pixl:%d\n", xImage1->bytes_per_pixel);
    if (height > xImage1->height || width > xImage1->width) {
       return;
    }

	// destroy scaler filter etc
	if (Scaled != img_ctx->pixFiltType) {
	    if (img_ctx->pixFilt)  {
	    	destroyPixFilt(img_ctx->pixFilt);
	    	img_ctx->pixFilt = (pixFilt_t *)0;
	    }
	    img_ctx->pixFiltType = Scaled;
	}

    if (Scaled) {
        // is there a scaler, needed size:
        int scalerSz = img_ctx->winW * img_ctx->winH * sizeof(uint32_t);
        if (scalerSz > img_ctx->scalerSz) {

            //fprintf(stderr, "alloc scaled img\n");
            if (img_ctx->scalerData) {
                free(img_ctx->scalerData);
            }
            img_ctx->scalerData = (uint32_t *)malloc(scalerSz);
            img_ctx->scalerSz = 0;
            if (img_ctx->scalerData) {
                img_ctx->scalerSz = scalerSz;
            }
        }
        // perform scaling
        if (img_ctx->scalerData) {

            switch(Scaled) {
               default:
               case 1:
                 image_scale
                      (img_ctx,
                       img_ctx->scalerData, img_ctx->winW, img_ctx->winH,
                       rgb32Frame,          width,         height);
                 break;
               case 2:
            	 if (!img_ctx->pixFilt) {
            		img_ctx->pixFilt = createPixFiltNearestNeighbor();
            	 }
                 image_scale_filtered
                      (img_ctx,
                       img_ctx->scalerData, img_ctx->winW, img_ctx->winH,
                       rgb32Frame,          width,         height);
                 break;
               case 3:
                 if (!img_ctx->pixFilt) {
                    img_ctx->pixFilt = createPixFiltBiLinear();
                 }
                 image_scale_filtered
                      (img_ctx,
                       img_ctx->scalerData, img_ctx->winW, img_ctx->winH,
                       rgb32Frame,          width,         height);
                 break;
            }
            // set input for the rest of this routine to the new (scaled) output
            rgb32Frame = img_ctx->scalerData;
            width      = img_ctx->winW;
            height     = img_ctx->winH;
        }
        else {
            perror("alloc");
        }
    }
    //memset(xImage1->data, 0, img_ctx->winH*xImage1->bytes_per_line);

    iSrcLineSz = width;
    iDstLineSz = xImage1->bytes_per_line;

    iSrc       = rgb32Frame;
    iDst       = (uint8_t *) xImage1->data;

    if ((xImage1->depth == 24) && (xImage1->bits_per_pixel == 32)) {
        for (y=0; y<height; y++) {
            memcpy_32(iDst, iSrc, iSrcLineSz);
            iDst += iDstLineSz;
            iSrc += iSrcLineSz;
        }
    }
    else
    if ((xImage1->depth == 24) && (xImage1->bits_per_pixel == 24))
    {
        for (y=0; y<height; y++) {
            memcpy_24(iDst, iSrc, iSrcLineSz);
            iDst += iDstLineSz;
            iSrc += iSrcLineSz;
        }
    }
    else
    if ((xImage1->depth == 16) && (xImage1->bits_per_pixel == 16))
    {
        for (y=0; y<height; y++) {
            memcpy_16(iDst, iSrc, iSrcLineSz);
            iDst += iDstLineSz;
            iSrc += iSrcLineSz;
        }
    }
    else {
        fprintf(stderr, "depth not supported\n");
        return;
    }

    {
        void boundingMask(Display *g_display, Window g_win, int width, int height, uint32_t *rgbaFrame);
        boundingMask(img_ctx->display,
                     img_ctx->window,
                     img_ctx->winW,
                     img_ctx->winH,
                     rgb32Frame);
    }

    // image_data_cache
    {
        int cache_sz = img_ctx->xImage->bytes_per_line * img_ctx->xImage->height;
        if (cache_sz > image_data_cache_sz) {
            if (image_data_cache && image_data_cache_sz) {
                free(image_data_cache);
                image_data_cache    = (char *)0;
                image_data_cache_sz = 0;
            }
            image_data_cache = (char *)malloc(cache_sz);
            if (image_data_cache) {
                image_data_cache_sz = cache_sz;
            }
        }
        if (image_data_cache  && image_data_cache_sz) {
            memcpy(image_data_cache, img_ctx->xImage->data, cache_sz);
        }
    }

    //fprintf(stderr, "put:w*h %d x %d\n", img_ctx->winW, img_ctx->winH);
    image_put(img_ctx, 0, 0, 0, 0, img_ctx->winW, img_ctx->winH);
    XSync(img_ctx->display, False);
}
// --------------------------------------------------------------------------

void boundingMask(Display *g_display, Window   g_win, int width, int height, uint32_t *rgbaFrame)
{
    GC mask_gc;
    Pixmap bitmap;

    //if (0) {
    //    FILE *ofp = fopen("bitmask.txt", "w");
    //    int x;
    //    int y;
    //    int p;
    //    p=0;
    //    for (y=0; y<height; y++) {
    //        for (x=0; x<width; x++) {
    //            if (((rgbaFrame[p++]&0xff000000)>>24) >= 0x80)
    //                fprintf(ofp, "1");
    //            else
    //                fprintf(ofp, "0");
    //        }
    //        fprintf(ofp,"\n");
    //    }
    //    fclose(ofp);
    //}


    // --------- Draw on mask --------
    #define LINE_WIDTH 20

    // Create a simple bitmap mask (depth = 1)
    bitmap = XCreatePixmap(g_display, g_win, width, height, 1);

    // Create mask_gc
    mask_gc = XCreateGC(g_display, bitmap, 0, NULL);
    XSetBackground(g_display, mask_gc, 1);

    if (1) {
        int x, y, p;

        // Set all the mask bits first
        XSetForeground(g_display, mask_gc, 1);
        XFillRectangle(g_display, bitmap, mask_gc, 0, 0, width, height);
        // Turn them selectively off, based on alfa
        XSetForeground(g_display, mask_gc, 0);
        p=0;
        for (y=0; y<height; y++) {
            for (x=0; x<width; x++) {
                if (((rgbaFrame[p++]&0xff000000)>>24) < 0x80) {
                    //fprintf(ofp, "0");
                    XDrawPoint(g_display, bitmap, mask_gc, x, y);
                }
            }
        }
    }

    // Set mask
    XShapeCombineMask(g_display, g_win, ShapeBounding, 0, 0, bitmap, ShapeSet);
    // Free objects
    XFreePixmap(g_display, bitmap);
    XFreeGC(g_display, mask_gc);
}

// ----------------------------------------------------------
// local helper functions for image_display
void memcpy_32(uint8_t *iDst, uint32_t *iSrc, int iSrcLineSz)
{
    memcpy(iDst, iSrc, iSrcLineSz*sizeof(uint32_t));
}
// ----------------------------------------------------------
void memcpy_24(uint8_t *iDst, uint32_t *iSrc, int iSrcLineSz)
{
    int i;
    for (i=0; i<iSrcLineSz; i++){
       *iDst++ = (uint8_t)(*iSrc >> 0);  // B
       *iDst++ = (uint8_t)(*iSrc >> 8);  // G
       *iDst++ = (uint8_t)(*iSrc >> 16); // R
       iSrc++;
    }
}
// ----------------------------------------------------------
void memcpy_16(uint8_t *iDst, uint32_t *iSrc, int iSrcLineSz)
{
    int i;
    uint8_t R,G,B;
    uint16_t *P;

    P = (uint16_t *)iDst;
    for (i=0; i<iSrcLineSz; i++){
       *P++ = ( (uint16_t)(*iSrc >> 8) & 0xf800 ) | // R 5bits (3...7) -> src:(19...23) ==> dst:(11...15)
              ( (uint16_t)(*iSrc >> 5) & 0x07e0 ) | // G 6bits (2...7) -> src:(10...15) ==> dst:( 5...10)
              ( (uint16_t)(*iSrc >> 3) & 0x001f );  // B 5bits (3...7) -> src:( 3...7)  ==> dst:( 0... 4)
       iSrc++;
    }
}
// ----------------------------------------------------------
// Scale the image: nearest neighbour pixel replication
void image_scale (image_context *img_ctx,
                  uint32_t *rgb32Out, int oWidth, int oHeight,
                  uint32_t *rgb32In,  int iWidth, int iHeight)
{
    // --------------------------
    // local persistent memory
    uint32_t  *w      = img_ctx->scl2Data;
    int       wSz     = img_ctx->scl2Sz;
    int       nSz;
    // --------------------------
    // quick redundancy check
    if ((oWidth == iWidth) && (oHeight == iHeight)) {
        memcpy(rgb32Out, rgb32In, iWidth*iHeight*sizeof(uint32_t));
        return;
    }
    // --------------------------
    // clean output
    // memset(rgb32Out, 0, oWidth*oHeight*sizeof(uint32_t));

    // --------------------------
    // check work buf: it must hold a picture oWidth*iHeight
    nSz = oWidth*iHeight*sizeof(uint32_t);

    if (nSz > wSz) {
        //fprintf(stderr, "image_scale: resize workbuf %dx%d\n", oWidth, iHeight);

        img_ctx->scl2Data = (uint32_t *)0;
        img_ctx->scl2Sz   = 0;

        if (w) {
            free(w);
            w = (uint32_t *)0;
        }
        wSz = 0;
        w = (uint32_t *)malloc(nSz);
        if (!w) {
           perror("image_scale: resize workbuf failed");
           return;
        }
        wSz = nSz;

        img_ctx->scl2Data = (uint32_t *)w;
        img_ctx->scl2Sz   = wSz;
    }

    // --------------------------
    // x scaling
    if (iWidth <= oWidth) {
        // x upscaling
        int x, y, frac;
        uint32_t *dst, *src;

        dst  = w;
        src  = rgb32In;
        frac = 0;
        for (y=0; y<iHeight; y++) {

            frac = 0;
            for(x=0; x<oWidth; x++) {

                // pixel replicate source
                *dst++ = *src;

                frac += iWidth;
                if (frac >= oWidth) {
                    src++;
                    frac -= oWidth;
                }
            }
        }
    }
    else {
        // x downscaling
        //fprintf(stderr, "x downscaling not supported yet.\n");
        //return;
        int x, y, frac;
        uint32_t *dst, *src;

        dst  = w;
        src  = rgb32In;
        frac = 0;
        for (y=0; y<iHeight; y++) {

            frac = 0;
            for(x=0; x<iWidth; x++) {

                frac += oWidth;
                if (frac >= iWidth) {

                    // pixel replicate source
                    *dst++ = *src;
                    frac -= iWidth;
                }
                src++;
            }
        }
    }

    // debug: temporary dump to output
    //memcpy(rgb32Out, w, iHeight*oWidth*sizeof(uint32_t));
    //return;


    // y scaling
    if (iHeight <= oHeight) {
        // y upscaling

        int x, y, frac;
        uint32_t *dst, *src;

        dst  = rgb32Out;
        src  = w;
        frac = 0;
        for (x=0; x<oWidth; x++) {

            frac = 0;
            src  = w        + x;
            dst  = rgb32Out + x;

            for(y=0; y<oHeight; y++) {

                // pixel replicate source
                *dst = *src;

                 // step to next line
                dst += oWidth;

                frac += iHeight;
                if (frac >= oHeight) {
                    // step to next input line
                    src  += oWidth;
                    frac -= oHeight;
                }
            }
        }
    }
    else {
        // y downscaling
        //fprintf(stderr, "y downscaling not supported yet.\n");
        //return;

        int x, y, frac;
        uint32_t *dst, *src;

        dst  = rgb32Out;
        src  = w;
        frac = 0;
        for (x=0; x<oWidth; x++) {

            frac = 0;
            src  = w        + x;
            dst  = rgb32Out + x;

            for(y=0; y<iHeight; y++) {

                frac += oHeight;
                if (frac >= iHeight) {

                    // pixel replicate source
                    *dst = *src;

                    // step to next output line
                    dst  += oWidth;
                    frac -= iHeight;
                }

                // step to next input line
                src += oWidth;
            }
        }
    }
}
// ----------------------------------------------------------
// Scale the image: nearest neighbour pixel replication
void image_scale_filtered (image_context *img_ctx,
                           uint32_t *rgb32Out, int oWidth, int oHeight,
                           uint32_t *rgb32In,  int iWidth, int iHeight)
{
    // --------------------------
    // local persistent memory
    uint32_t  *w      = img_ctx->scl2Data;
    int       wSz     = img_ctx->scl2Sz;
    int       nSz;
    // Pre: Filter must be initialized
    pixFilt_t *F;


    // --------------------------
    // quick redundancy check
    if ((oWidth == iWidth) && (oHeight == iHeight)) {
        memcpy(rgb32Out, rgb32In, iWidth*iHeight*sizeof(uint32_t));
        return;
    }
    //printf("image_scale_bilinear\n");
    F=img_ctx->pixFilt; // createPixFiltBiLinear();
    if (!F)
    	return;

    // --------------------------
    // clean output
    // memset(rgb32Out, 0, oWidth*oHeight*sizeof(uint32_t));

    // --------------------------
    // check work buf: it must hold a picture oWidth*iHeight
    nSz = oWidth*iHeight*sizeof(uint32_t);

    if (nSz > wSz) {
        //fprintf(stderr, "image_scale: resize workbuf %dx%d\n", oWidth, iHeight);

        img_ctx->scl2Data = (uint32_t *)0;
        img_ctx->scl2Sz   = 0;

        if (w) {
            free(w);
            w = (uint32_t *)0;
        }
        wSz = 0;
        w = (uint32_t *)malloc(nSz);
        if (!w) {
           perror("image_scale: resize workbuf failed");
           return;
        }
        wSz = nSz;

        img_ctx->scl2Data = (uint32_t *)w;
        img_ctx->scl2Sz   = wSz;
    }

    // --------------------------
    // x scaling
    if (iWidth <= oWidth) {
        // x upscaling
        int x, y;
        uint32_t *dst, *src, *rtl;

        int      flen2_0;  // run in length
        int      flen2_1;  // run out length
        uint32_t ffrac_0;  // start frac used for filter
        uint32_t ffrac_i;  // increment frac used for filter (note: 0 means 1.0: 0x1-00000000)
        uint32_t ffrac_p;  // current frac
        uint32_t ffrac_pp; // previous frac
        uint32_t ffrac_f;  // current frac filter phase

        dst  = w;
        src  = rgb32In;

        flen2_0 = F->tapsNr/2;
        flen2_1 = F->tapsNr - flen2_0;
        ffrac_0 = flen2_1 == flen2_0 ? 0xffffffff : 0x00000000;
        ffrac_i = (uint32_t)( ((uint64_t)iWidth << 32) / ((uint64_t)oWidth) );

        for (y=0; y<iHeight; y++) {

            // filt Run in part one: first SRC has to be in the middle
            rtl = src + (int)flen2_0;
            for(x=flen2_0; x; x--) pixFiltIn(F, *rtl--);

            // prepare run out pointer (point to last pix of input line)
            rtl = src + (int)(iWidth-1);

            // filt Run in part two: first SRC has to be in the middle
            for(x=flen2_1; x; x--) pixFiltIn(F, *src++);

            ffrac_p  = ffrac_0;

            for(x=0; x<oWidth; x++) {

                // pixel replicate source
                // *dst++ = *src;
                ffrac_f = (uint32_t)(((uint64_t)F->polyNr * (uint64_t)ffrac_p)>>32);
                *dst++  = pixFiltOut(F, (int)ffrac_f);

                ffrac_pp = ffrac_p;
                ffrac_p += ffrac_i;

                //printf("x:%3d _f:%3d, _p:0x%08x, src:%p, rtl:%p\n", x, ffrac_f, ffrac_p, src, rtl);

                if (ffrac_p < ffrac_pp) {
                    if (src <= rtl) {
                        // forwards
                        pixFiltIn(F, *src++);
                    }
                    else {
                        // run out
                        pixFiltIn(F, *--rtl);
                    }
                }
            }
            //getchar();
        }
    }
    else {
        // x downscaling
        //fprintf(stderr, "x downscaling not supported yet.\n");
        //return;
        int x, y, frac;
        uint32_t *dst, *src;

        dst  = w;
        src  = rgb32In;
        frac = 0;
        for (y=0; y<iHeight; y++) {

            frac = 0;
            for(x=0; x<iWidth; x++) {

                frac += oWidth;
                if (frac >= iWidth) {

                    // pixel replicate source
                    *dst++ = *src;
                    frac -= iWidth;
                }
                src++;
            }
        }
    }

    // debug: temporary dump to output
    //memcpy(rgb32Out, w, iHeight*oWidth*sizeof(uint32_t));
    //return;


    // y scaling
    if (iHeight <= oHeight) {
        // y upscaling

//------
        // y upscaling
        int x, y;
        uint32_t *dst, *src, *rtl;

        int      flen2_0;  // run in length
        int      flen2_1;  // run out length
        uint32_t ffrac_0;  // start frac used for filter
        uint32_t ffrac_i;  // increment frac used for filter (note: 0 means 1.0: 0x1-00000000)
        uint32_t ffrac_p;  // current frac
        uint32_t ffrac_pp; // previous frac
        uint32_t ffrac_f;  // current frac filter phase

        flen2_0 = F->tapsNr/2;
        flen2_1 = F->tapsNr - flen2_0;
        ffrac_0 = flen2_1 == flen2_0 ? 0xffffffff : 0x00000000;
        ffrac_i = (uint32_t)( ((uint64_t)iHeight << 32) / ((uint64_t)oHeight) );

        for (x=0; x<oWidth; x++) {

            src  = w        + x;
            dst  = rgb32Out + x;

            // filt Run in part one: first SRC has to be in the middle
            rtl = src + (int)(flen2_0 * oWidth);
            for(y=flen2_0; y; y--) { pixFiltIn(F, *rtl); rtl -= oWidth; }

            // prepare run out pointer (point to last pix of input line)
            rtl = src + (int)((iHeight-1) * oWidth);

            // filt Run in part two: first SRC has to be in the middle
            for(y=flen2_1; y; y--) { pixFiltIn(F, *src); src += oWidth; }

            ffrac_p  = ffrac_0;

            for(y=0; y<oHeight; y++) {

                // pixel replicate source
                // *dst = *src; *dst+=oWidth
                ffrac_f = (uint32_t)(((uint64_t)F->polyNr * (uint64_t)ffrac_p)>>32);
                *dst    = pixFiltOut(F, (int)ffrac_f);

                dst += oWidth;

                ffrac_pp = ffrac_p;
                ffrac_p += ffrac_i;

                // printf("x:%4d y:%3d _f:%3d, _p:0x%08x, src:%p, rtl:%p\n", x, y, ffrac_f, ffrac_p, src, rtl);

                if (ffrac_p < ffrac_pp) {
                    if (src <= rtl) {
                        // forwards
                        pixFiltIn(F, *src);
                        src += oWidth;
                    }
                    else {
                        // run out
                    	rtl -= oWidth;
                        pixFiltIn(F, *rtl);
                    }
                }
            }
            //getchar();
        }
    }
    else {
        // y downscaling
        //fprintf(stderr, "y downscaling not supported yet.\n");
        //return;

        int x, y, frac;
        uint32_t *dst, *src;

        dst  = rgb32Out;
        src  = w;
        frac = 0;
        for (x=0; x<oWidth; x++) {

            frac = 0;
            src  = w        + x;
            dst  = rgb32Out + x;

            for(y=0; y<iHeight; y++) {

                frac += oHeight;
                if (frac >= iHeight) {

                    // pixel replicate source
                    *dst = *src;

                    // step to next output line
                    dst  += oWidth;
                    frac -= iHeight;
                }

                // step to next input line
                src += oWidth;
            }
        }
    }
}
// ----------------------------------------------------------
void image_showinfo(image_context    *img_ctx)
{
    const char *ByteOrderName(int byteOrder);
    const char *VisualClassName(int visualClass);

    fprintf(stderr,"\nDisplay:\n");
    fprintf(stderr,"Image byte order = %s\n", ByteOrderName(ImageByteOrder(img_ctx->display)));
    fprintf(stderr,"Bitmap unit      = %i\n", BitmapUnit(img_ctx->display));
    fprintf(stderr,"Bitmap bit order = %s\n", ByteOrderName(BitmapBitOrder(img_ctx->display)));
    fprintf(stderr,"Bitmap pad       = %i\n", BitmapPad(img_ctx->display));

    fprintf(stderr,"\nWindow:\n");
    fprintf(stderr,"Depth            = %i\n", img_ctx->windowAttributes.depth);
    //fprintf(stderr,"Visual ID        = 0x%02x\n", img_ctx.windowAttributes.visual->visualid);
    //fprintf(stderr,"Visual class     = %s\n",
    //              VisualClassName(img_ctx.windowAttributes.visual->c_class));
    fprintf(stderr,"Red mask         = 0x%08lx\n", img_ctx->windowAttributes.visual->red_mask);
    fprintf(stderr,"Green mask       = 0x%08lx\n", img_ctx->windowAttributes.visual->green_mask);
    fprintf(stderr,"Blue mask        = 0x%08lx\n", img_ctx->windowAttributes.visual->blue_mask);
    fprintf(stderr,"Bits per R/G/B   = %i\n",      img_ctx->windowAttributes.visual->bits_per_rgb);

    fprintf(stderr,"Events           = 0x%08lx\n", img_ctx->windowAttributes.your_event_mask);

    fprintf(stderr,"Image byte order = %s\n", ByteOrderName((img_ctx->xImage)->byte_order));
    fprintf(stderr,"Bitmap unit      = %i\n", img_ctx->xImage->bitmap_unit);
    fprintf(stderr,"Bitmap bit order = %s\n", ByteOrderName(img_ctx->xImage->bitmap_bit_order));
    fprintf(stderr,"Bitmap pad       = %i\n", img_ctx->xImage->bitmap_pad);
    fprintf(stderr,"Depth            = %i\n", img_ctx->xImage->depth);
    fprintf(stderr,"Red mask         = 0x%08lx\n", img_ctx->xImage->red_mask);
    fprintf(stderr,"Green mask       = 0x%08lx\n", img_ctx->xImage->green_mask);
    fprintf(stderr,"Blue mask        = 0x%08lx\n", img_ctx->xImage->blue_mask);
    fprintf(stderr,"Bits per pixel   = %i\n", img_ctx->xImage->bits_per_pixel);
    fprintf(stderr,"Bytes per line   = %i\n", img_ctx->xImage->bytes_per_line);
    fprintf(stderr,"IsShared         = %s\n", img_ctx->isShared ? "True" : "False");
    //fprintf(stderr,"HasSharedPixmap  = %s\n", img_ctx.HasSharedPixmap() ? "True" : "False");
}
// ----------------------------------------------------------
// functions local to image_showinfo
const char *ByteOrderName(int byteOrder)
{
    switch (byteOrder)
    {
        case LSBFirst: return ("LSBFirst");
        case MSBFirst: return ("MSBFirst");
        default:       return ("?");
    }
}
// ----------------------------------------------------------
const char *VisualClassName(int visualClass)
{
    switch (visualClass)
    {
        case StaticGray:  return ("StaticGray");
        case GrayScale:   return ("GrayScale");
        case StaticColor: return ("StaticColor");
        case PseudoColor: return ("PseudoColor");
        case TrueColor:   return ("TrueColor");
        case DirectColor: return ("DirectColor");
        default:          return ("?");
    }
}

// ----------------------------------------------------------
// manages window, handles all events, if there are any
// returns true if program wants to run (false on exit)

int image_manage (image_context *img_ctx) // handles all events
{
    int Rcontinue = true; // default: keep runnin...

    // recursive call block gate
    //if (G_in_image_manage)
    //    return true;

    G_in_image_manage = true;

    while (XPending(img_ctx->display) > 0) {
        XNextEvent(img_ctx->display, &img_ctx->event); //refresh the picture

        // shortcut to import somebody else's code
        XEvent           xevt = img_ctx->event;
        XExposeEvent    *eev;
        XConfigureEvent *cev;
        XKeyEvent       *kev;
        XMotionEvent    *xmotion;
        KeySym          key_symbol;
        XPoint          xp;  // Mouse point

        //event_mask =  StructureNotifyMask | ExposureMask    |
        //              PropertyChangeMask  | EnterWindowMask | LeaveWindowMask | KeyRelease | ButtonPress |
        //              ButtonRelease       | KeymapStateMask

        switch(img_ctx->event.type) {


            // belonging to StructureNotifyMask
            case CirculateNotify:
            case DestroyNotify:
            case GravityNotify:
            case MapNotify:
            case ReparentNotify:
            case UnmapNotify:
            break;
            case ConfigureNotify:
                {
                    XConfigureEvent *R = (XConfigureEvent *)&(img_ctx->event);
                    //fprintf(stderr, "Configure   serial:%d, send:%d, w:%d, h:%d\n", R->serial, R->send_event, R->width, R->height);
                    // query new size
                    XGetWindowAttributes(img_ctx->display, img_ctx->window, &(img_ctx->windowAttributes));
                    img_ctx->winW  = R->width;
                    img_ctx->winH  = R->height;

                    //fprintf(stderr, "configure:(resize: %d x %d)\n", R->width, R->height);
                }
                break;

            case Expose:
                {
                    XExposeEvent *R = (XExposeEvent *)&(img_ctx->event);
                    //fprintf(stderr, "Expose serial:%d, send:%d, w:%d, h:%d cnt:%d\n",
                    //        R->serial, R->send_event, R->width, R->height, R->count);
                    // query new size
                    XGetWindowAttributes(img_ctx->display, img_ctx->window, &(img_ctx->windowAttributes));
                    img_ctx->winW  = img_ctx->windowAttributes.width;
                    img_ctx->winH  = img_ctx->windowAttributes.height;

                    // utterly wrong: these are exposure regions
                    // img_ctx->winW  = R->width;
                    // img_ctx->winH  = R->height;
                    // ==================================================================
                    // do not refresh display if another exposure is coming up
                    if (XPending(img_ctx->display) > 0) {
                        // peek next event
                        XEvent E;
                        XPeekEvent(img_ctx->display, &E);
                        if (E.type == Expose)
                            continue;
                    }

                    if (!img_ctx->srcFrame)
                    {
                    image_put(img_ctx,
                              0, 0, // src x,y
                              0, 0, // tgt x,y
                              img_ctx->winW, img_ctx->winH); // width and height of drawing
                    }
                    else {
                        // image_display (img_ctx,
                        //                img_ctx->srcFrame,
                        //                img_ctx->srcWidth,
                        //                img_ctx->srcHeight,
                        //                img_ctx->srcScaleOption); // rescale source and reput
                        // Refresh is now done in the main loop with machine stretching
                    }
                    //XSync(img_ctx->display, False);
                    //fprintf(stderr, "expose:(size: %d x %d)\n", img_ctx->winW, img_ctx->winH);

                }
                break;


            case PropertyNotify:
                fprintf(stderr,"PropertyNotify\n");
                break;


            case EnterNotify:
                fprintf(stderr,"Enter window\n");
                // Quit!
                //exit(1);
                break;

            case LeaveNotify:
                fprintf(stderr,"Leave window\n");
                // Quit!
                //exit(1);
                break;


            case ButtonPress:
                //fprintf(stderr,"Got Button press\n");
                // Quit!
                //exit(1);
                xp.x = xevt.xbutton.x;
                xp.y = xevt.xbutton.y;

                pKnobDetails K;

                // --------------------------------
                // release first?
                K = img_ctx->machine_knob_pressed;
                if (K && K->release) {
                    K->release(K);
                }
                img_ctx->machine_knob_pressed = 0;

                // --------------------------------
                // find out if we press a knob of the machine
                K = (img_ctx->machine ? img_ctx->machine->knobList : 0);
                for (; K && K->name[0] ; K++) {
                    if (!K->circle &&
                        xp.x >= K->tlx &&
                        xp.x <= K->brx &&
                        xp.y >= K->tly &&
                        xp.y <= K->bry ||
                        K->circle &&
                        ((xp.x - K->tlx)*(xp.x - K->tlx)+(xp.y - K->tly)*(xp.y - K->tly) <= K->brx*K->brx)
                    ) {
                        img_ctx->machine_knob_pressed = K;
                    }
                }
                // knob pressed?
                K = img_ctx->machine_knob_pressed;
                if (K && K->press) {
                    K->press(K);
                }

                // -----------------------------------------------------------------------
                // override any knob callback function,
                // for things that are done more easily over here (having all data around)
                // Pressing in the top left corner, will toggle the window between decorated to non-decorated.
                // when decorated: it is resizeable and moveable
                // when non decorated, it looks like a splash screen

                if (K &&!strcmp(K->name, "TLcorn"))
                {
                    int xmem, ymem;

                    G_decoration_state = ! G_decoration_state;

                    //When going from no decoration, to decoration, we have to unmap and map the window, otherwise cinnamon or other wm's will go astray...

                    if (G_decoration_state) {
                        // remember x,y of window
                        XWindowAttributes A;
                        XGetWindowAttributes(img_ctx->display, img_ctx->window, &A);
                        XUnmapWindow(img_ctx->display, img_ctx->window);
                        xmem = A.x;
                        ymem = A.y;
                        fprintf(stderr, "decoration: x,y=%d,%d\n", xmem,ymem);
                    }

                    image_decoration(img_ctx, G_decoration_state);

                    if (G_decoration_state) {

                        // When remapping the window, can be in the middle due to the WM.
                        // So we move the window back to the original place.
                        // However, when remapping the window to a decorated state, the window border and caption height
                        // will not restore it to the same position.
                        // therefore we use the _NET_FRAME_EXTENTS extents..

                        XMapWindow(img_ctx->display, img_ctx->window);

                        image_context_props E;
                        E = image_get_extents(img_ctx);
                        XMoveWindow(img_ctx->display, img_ctx->window, xmem-E.winW, ymem-E.winH);

                        fprintf(stderr, "moving: x,y=%d,%d:\n", xmem,ymem);

                        // after map: hardcode release the button, since the button release event wont come
                        if (K->release) {
                            fprintf(stderr, "releasing K:\n");
                            K->release(K);
                        }
                        img_ctx->machine_knob_pressed = 0;
                    }

                    fprintf(stderr, "decoration:%d\n", G_decoration_state);
                    // Rcontinue = false;
                }

                break;

            case ButtonRelease:

                //pKnobDetails K;
                // --------------------------------
                // if knob from the knoblist, release first
                K = img_ctx->machine_knob_pressed;
                if (K && K->release) {
                    K->release(K);
                }
                img_ctx->machine_knob_pressed = 0;

                // override any function, for things done more easily here
                if (K && !strcmp(K->name, "Power")) {
                        Rcontinue = false;
                }

                //fprintf(stderr,"Got Button release\n");
                break;

            case MotionNotify:
                //fprintf(stderr,"Got motion\n");

                xmotion = &xevt.xmotion;

                xp.x = xevt.xbutton.x;
                xp.y = xevt.xbutton.y;
                // --------------------------------
                // if knob from the knoblist, drag function
                K = img_ctx->machine_knob_pressed;
                if (K && K->drag) {
                    K->drag(K, xp.x, xp.y);
                }
                else {
                    // else print drag coords, handy for making new knobs
                    if (xmotion->state & Button1Mask)
                    {
                        fprintf(stderr,"drag b1:%d %d\n", xp.x, xp.y);
                        // moveWindow2(xp);
                    }
                }
                break;


            case KeymapNotify:
            // To receive KeymapNotify events, set the KeymapStateMask bit in the event-mask attribute of the window.
            // The X server generates this event immediately after every EnterNotify and FocusIn event.
            break;

            case KeyPress:
            case KeyRelease:

                kev = &xevt.xkey;
                int x, y;
                Window the_win;

                char *kA;
                kA = (char *) ((img_ctx->event.type == KeyRelease) ? "release" : "press");

                x = kev->x;
                y = kev->y;
                the_win = kev->window;

                // Keycode to a keysymbol.
                // deprecated
                //key_symbol = XKeycodeToKeysym(img_ctx->display, kev->keycode, 0);
                // replacement
                {
                    int nrKeysyms;
                    KeySym *keysym = XGetKeyboardMapping(img_ctx->display, kev->keycode, 1, &nrKeysyms);
                    key_symbol = keysym[0]; // do something with keysym[0]
                    XFree(keysym);
                }

                switch (key_symbol)
                {
                    case XK_1:
                    case XK_KP_1:
                        fprintf(stderr,"Key %s: '1' \n", kA);
                        break;

                    case XK_Delete:
                        fprintf(stderr,"Key %s: 'delete' \n", kA);
                        break;

                    case XK_Left:
                        //moveWindow(-5, 0);
                        break;

                    case XK_Right:
                        //moveWindow(5, 0);
                        break;

                    case XK_Down:
                        //moveWindow(0, 5);
                        break;

                    case XK_Up:
                        //moveWindow(0, -5);
                        break;

                    default:  /* anything else - check if it is a letter key */

                        int ascii_key;
                        if (key_symbol >= XK_A && key_symbol <= XK_Z)
                        {
                            ascii_key = key_symbol - XK_A + 'A';
                            // printf("Key %s - '%c'\n", kA, ascii_key);
                        }
                        else if (key_symbol >= XK_a && key_symbol <= XK_z)
                        {
                            ascii_key = key_symbol - XK_a + 'a';
                            // printf("Key %s - '%c'\n", kA, ascii_key);
                        }

                        switch (toupper(ascii_key))
                        {
                            case 'U':
                                fprintf(stderr,"Key %s: 'U' \n", kA);
                                break;

                            case 'Q':
                                Rcontinue = false;
                                break;

                            case 'N':
                                break;
                        }
                }
                break;

            default:
                fprintf(stderr, "Got event: but dunno what: 0x%08x\n", (int)img_ctx->event.type);
                break;
        }
    }
    G_in_image_manage = false;
    return Rcontinue;
}


void NkPutPixel (XImage *img, int x, int y, int p)
{
    uint32_t pix1,pix2,pix3;

    uint32_t p1c0, p1c1, p2c0, p2c1, p3c0, p3c1;


    // pix3 = 0x001b3920; //  darkest
    // pix3 = 0x002b5b33; //  darkest
    pix1 = 0x003d8048; //  darkest original
    pix3 = 0x0032693b; //  darkest

    // pix1 = 0x003ba65c;
    // pix1 = 0x003d8048; //  darkest

    if (p != 0) {
        pix1 = 0x00c8a014; // light amber
        pix3 = 0x00a48311; // amber
    }

    if (G_is_scope) {
     //pix1 = 0x0054c477; //  lighter
     //pix3 = 0x0072d793; //  lightest
     pix3 = 0x00edfffd;
     pix1 = pix3;
    }
    pix2 = pix1-0x00000100;

    // compare pixels
    {
        p1c0 = 0x003d8048; p1c1 = 0x00c8a014;
        p2c0 = 0x003d7f48; p2c1 = 0x00c89f14;
        p3c0 = 0x0032693b; p3c1 = 0x00a48311;
    }


    int opix = XGetPixel(img, x, y);
    int npix = pix1;

    if ((opix == p1c0) || (opix == p1c1))
        npix = pix2;
    else
        if ((opix == p2c0) || (opix == p2c1))
            npix = pix3;
        else
            if ((opix == p3c0) || (opix == p3c1))
                npix = pix3;

    XPutPixel(img, x, y, npix);
}

void image_drawdata (image_context *img_ctx,
                     float *buf1,
                     float *buf2,
                     int   buf_sz,
                     int   disp_x, int disp_y,
                     int   disp_w, int disp_h,
                     int   mach_w, int mach_h)

{
    if (!image_data_cache || !image_data_cache_sz)
        return;

    memcpy(img_ctx->xImage->data, image_data_cache, image_data_cache_sz);

    // original screen sizes, to scaled sizes
#define BPIXS 3
    float xo=(float)(disp_x+BPIXS)    * (float)img_ctx->winW/(float)mach_w;
    float yo=(float)(disp_y+BPIXS)    * (float)img_ctx->winH/(float)mach_h;
    float xw=(float)(disp_w-2*BPIXS)  * (float)img_ctx->winW/(float)mach_w;
    float yw=(float)(disp_h-2*BPIXS)  * (float)img_ctx->winH/(float)mach_h;

    //if (G_is_scope) {
    //}

    float xd=xw/(float)buf_sz;

    // traces are placed at y = 1/3, y=2/3
    // scaling a range of 1 occupies 2/3 of screen
    // with offsets of -0.5, traces are placed at 0, 1/3

    float yd= yw ; //* 2.0/3.0;

    // hp8591 trace colors
    //uint32_t pix = 0x0015b24d; //  own guess
    //uint32_t pix1 = 0x003d8048; //  darkest
    //uint32_t pix2 = 0x0054c477; //  lighter
    //uint32_t pix3 = 0x0072d793; //  lightest

    // philips scope colors
    //uint32_t pix = 0x007ed2ec; //  darkest
    //uint32_t pix = 0x0093dae1; //  lighter
    //uint32_t pix3 = 0x008ee1e0; //  lightest

    // amber colors
    //uint32_t pix2 = 0x00a48311;

    float x;
    float y1, y2;
    int   i;
    int   y1_clip, y2_clip;

    int   xold = 0;
    int   xold_cnt = 0;


    int prv_x = -2;
    int prv_y1 = 0;
    int prv_y2 = 0;

    for (i=0; i<buf_sz; i++) {
        x = xo+xd*(float)i;

/*
        if ((int)x == xold) {
            xold = x;
            xold_cnt++;
            if (xold_cnt > 1000) {
               continue;
            }
        }
        else {
            xold=x;
            xold_cnt = 0;
        }
*/
        y1 = yo+0.5*yw - buf1[i]*yd;
        y2 = yo+0.5*yw - buf2[i]*yd;

        y1_clip = false;
        y2_clip = false;
        if (y1<yo)    {y1=yo;    y1_clip=true;}
        if (y1>yo+yw) {y1=yo+yw; y1_clip=true;}
        if (y2<yo)    {y2=yo;    y2_clip=true;}
        if (y2>yo+yw) {y2=yo+yw; y2_clip=true;}


        int new_x = x;
        int new_y, old_y, dlt_y, hlf_y;

        if (prv_x >= 0) {

            // draw vertline y1
            new_y = (int)y1;
            old_y = prv_y1;
            dlt_y = (new_y > old_y) ? 1 : -1;
            hlf_y = (old_y+new_y)/2;

            for(; old_y != hlf_y; old_y+=dlt_y) {
                NkPutPixel(img_ctx->xImage, prv_x, old_y, 1);
            }

            for(; old_y != new_y; old_y+=dlt_y) {
                NkPutPixel(img_ctx->xImage, new_x, old_y, 1);
            }
            if (!y1_clip)
            {
                NkPutPixel(img_ctx->xImage, new_x, old_y, 1);
            }

            // draw vertline y2
            new_y = (int)y2;
            old_y = prv_y2;
            dlt_y = (new_y > old_y) ? 1 : -1;
            hlf_y = (old_y+new_y)/2;

            for(; old_y != hlf_y; old_y+=dlt_y) {
                NkPutPixel(img_ctx->xImage, prv_x, old_y, 0);
            }

            for(; old_y != new_y; old_y+=dlt_y) {
                NkPutPixel(img_ctx->xImage, new_x, old_y, 0);
            }
            if (!y2_clip)
            {
                NkPutPixel(img_ctx->xImage, new_x, old_y, 0);
            }

        }

        prv_x  = new_x;
        prv_y1 = (int)y1;
        prv_y2 = (int)y2;
    }
    //fprintf(stderr, "put:w*h %d x %d\n", img_ctx->winW, img_ctx->winH);
    image_put(img_ctx, 0, 0, 0, 0, img_ctx->winW, img_ctx->winH);
    XSync(img_ctx->display, False);
}

// ----------------------------------------------------------
image_context_props image_get_props(image_context *img_ctx)
{
    image_context_props R;
    R.winW = img_ctx->winW;
    R.winH = img_ctx->winH;
    return R;
}

// ----------------------------------------------------------
// Window manager get extents,
// the border widths around the decorated window

image_context_props image_get_extents(image_context *img_ctx)
{

    Atom          propAtom, typeAtom;
    int           actualFmt;
    unsigned long nItems, nBytes;
    unsigned char *data = 0;
    long*         extents;
    XEvent        ev;

    image_context_props R;
    R.winW = 0;
    R.winH = 0;

    fprintf (stderr, "Getting extents\n");

    propAtom = XInternAtom(img_ctx->display, "_NET_FRAME_EXTENTS", True); /* Property to check */

    /* Window manager set up the extents immediately */
    /* Could wait until they are set up and there are 4 of them */
    /* But no, simply return 0, if it fails */

    if (XGetWindowProperty(img_ctx->display, img_ctx->window,
                              propAtom, 0, 4, False,
                              AnyPropertyType, &typeAtom, &actualFmt,
                              &nItems, &nBytes, &data) != Success || nItems != 4)
    {
        //fprintf (stderr, "Waiting for extents\n");
        //XNextEvent(img_ctx->display, &ev);
        fprintf (stderr, "No extents\n");
        return R;
    }

    /* OK got extents */
    extents = (long*) data;
    fprintf (stderr, "Got frame extents: left %ld right %ld top %ld bottom %ld\n",
            extents[0], extents[1], extents[2], extents[3]);

    // misuse winW, winH, for x, y offset of window borders
    R.winW = extents[1];
    R.winH = extents[2];
    return R;
}



#ifdef TEST_MAIN
// ----------------------------------------------------------
int main (int argc, char ** argv)
{
    int           width;
    int           height;
    uint32_t      *videoFrame;
    image_context *img_ctx;

    int i;

    img_ctx = image_init (640, 480, "MrMessy's cam");
    width  = 320;
    height = 240;

    videoFrame = (uint32_t*) malloc (width*height*sizeof(uint32_t));
    i = 0;

    {
       FILE *ifp;
       int rc;

       do {
          ifp = fopen("f.f", "rb");
          if (!ifp) {
             perror("fopen");
             exit(1);
          }
          while ((rc =fread(videoFrame, width*height*sizeof(uint32_t), 1, ifp)) == 1) {
             //fprintf(stderr, "process image\n");
             image_display (img_ctx, videoFrame, width, height);

             //getchar();
             //while(1) {
             //   if (XPending(img_ctx.display) > 0)
             //       XNextEvent(img_ctx.display, img_ctx.event); //refresh the picture
             //}
             //getchar();
          }
          //fprintf(stderr, "\nrc = %d rewind\n\n", rc);

          fclose(ifp);
          i++;
       } while (rc == 0 && i<1000);
    }

    free(videoFrame);
    image_destroy(img_ctx);
    printf("done\n");
    getchar();
    exit (0);
}
#endif
