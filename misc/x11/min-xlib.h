/*

Copyright 1985, 1986, 1987, 1991, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/


/*
 *	Xlib.h - Header definition and support file for the C subroutine
 *	interface library (Xlib) to the X Window System Protocol (V11).
 *	Structures and symbols starting with "_" are private to the library.
 */
#ifndef _X11_XLIB_H_
#define _X11_XLIB_H_

#define XlibSpecificationRelease 6

#include <sys/types.h>

#if defined(__SCO__) || defined(__UNIXWARE__)
#include <stdint.h>
#endif

#include "min-X.h"

#ifndef _XFUNCPROTOBEGIN
#if defined(__cplusplus) || defined(c_plusplus) /* for C++ V2.0 */
#define _XFUNCPROTOBEGIN extern "C" {	/* do not leave open across includes */
#define _XFUNCPROTOEND }
#else
#define _XFUNCPROTOBEGIN
#define _XFUNCPROTOEND
#endif
#endif /* _XFUNCPROTOBEGIN */


/* API mentioning "UTF8" or "utf8" is an XFree86 extension, introduced in
   November 2000. Its presence is indicated through the following macro. */
#define X_HAVE_UTF8_STRING 1

/* The Xlib structs are full of implicit padding to properly align members.
   We can't clean that up without breaking ABI, so tell clang not to bother
   complaining about it. */
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif

typedef char *XPointer;

#define Bool int
#define Status int
#define True 1
#define False 0

#define QueuedAlready 0
#define QueuedAfterReading 1
#define QueuedAfterFlush 2

#define RootWindow(dpy, scr) 	(ScreenOfDisplay(dpy,scr)->root)
#define DefaultScreen(dpy) 	(((_XPrivDisplay)(dpy))->default_screen)
#define DefaultRootWindow(dpy) 	(ScreenOfDisplay(dpy,DefaultScreen(dpy))->root)
#define DefaultVisual(dpy, scr) (ScreenOfDisplay(dpy,scr)->root_visual)
#define DefaultGC(dpy, scr) 	(ScreenOfDisplay(dpy,scr)->default_gc)
#define BlackPixel(dpy, scr) 	(ScreenOfDisplay(dpy,scr)->black_pixel)
#define WhitePixel(dpy, scr) 	(ScreenOfDisplay(dpy,scr)->white_pixel)
#define DisplayWidth(dpy, scr) 	(ScreenOfDisplay(dpy,scr)->width)
#define DisplayHeight(dpy, scr) (ScreenOfDisplay(dpy,scr)->height)
#define ScreenCount(dpy) 	(((_XPrivDisplay)(dpy))->nscreens)
#define DisplayString(dpy) 	(((_XPrivDisplay)(dpy))->display_name)
#define DefaultDepth(dpy, scr) 	(ScreenOfDisplay(dpy,scr)->root_depth)
#define DefaultColormap(dpy, scr)(ScreenOfDisplay(dpy,scr)->cmap)

/* macros for screen oriented applications (toolkit) */
#define ScreenOfDisplay(dpy, scr)(&((_XPrivDisplay)(dpy))->screens[scr])
#define DefaultScreenOfDisplay(dpy) ScreenOfDisplay(dpy,DefaultScreen(dpy))
#define DisplayOfScreen(s)	((s)->display)
#define RootWindowOfScreen(s)	((s)->root)
#define BlackPixelOfScreen(s)	((s)->black_pixel)
#define WhitePixelOfScreen(s)	((s)->white_pixel)
#define DefaultColormapOfScreen(s)((s)->cmap)
#define DefaultDepthOfScreen(s)	((s)->root_depth)
#define DefaultGCOfScreen(s)	((s)->default_gc)
#define DefaultVisualOfScreen(s)((s)->root_visual)
#define WidthOfScreen(s)	((s)->width)
#define HeightOfScreen(s)	((s)->height)

/*
 * Extensions need a way to hang private data on some structures.
 */
typedef struct _XExtData {
	int number;		/* number returned by XRegisterExtension */
	struct _XExtData *next;	/* next item on list of data for structure */
	int (*free_private)(	/* called to free private storage */
	struct _XExtData *extension
	);
	XPointer private_data;	/* data private to this extension. */
} XExtData;

/*
 * Graphics context.  The contents of this structure are implementation
 * dependent.  A GC should be treated as opaque by application code.
 */

typedef struct _XGC
#ifdef XLIB_ILLEGAL_ACCESS
{
    XExtData *ext_data;	/* hook for extension to hang data */
    GContext gid;	/* protocol ID for graphics context */
    /* there is more to this structure, but it is private to Xlib */
}
#endif
*GC;

/*
 * Visual structure; contains information about colormapping possible.
 */
typedef struct {
	XExtData *ext_data;	/* hook for extension to hang data */
	VisualID visualid;	/* visual id of this visual */
#if defined(__cplusplus) || defined(c_plusplus)
	int c_class;		/* C++ class of screen (monochrome, etc.) */
#else
	int class;		/* class of screen (monochrome, etc.) */
#endif
	unsigned long red_mask, green_mask, blue_mask;	/* mask values */
	int bits_per_rgb;	/* log base 2 of distinct color values */
	int map_entries;	/* color map entries */
} Visual;

/*
 * Depth structure; contains information for each possible depth.
 */
typedef struct {
	int depth;		/* this depth (Z) of the depth */
	int nvisuals;		/* number of Visual types at this depth */
	Visual *visuals;	/* list of visuals possible at this depth */
} Depth;

/*
 * Information about the screen.  The contents of this structure are
 * implementation dependent.  A Screen should be treated as opaque
 * by application code.
 */

struct _XDisplay;		/* Forward declare before use for C++ */

typedef struct {
	XExtData *ext_data;	/* hook for extension to hang data */
	struct _XDisplay *display;/* back pointer to display structure */
	Window root;		/* Root window id. */
	int width, height;	/* width and height of screen */
	int mwidth, mheight;	/* width and height of  in millimeters */
	int ndepths;		/* number of depths possible */
	Depth *depths;		/* list of allowable depths on the screen */
	int root_depth;		/* bits per pixel */
	Visual *root_visual;	/* root visual */
	GC default_gc;		/* GC for the root root visual */
	Colormap cmap;		/* default color map */
	unsigned long white_pixel;
	unsigned long black_pixel;	/* White and Black pixel values */
	int max_maps, min_maps;	/* max and min color maps */
	int backing_store;	/* Never, WhenMapped, Always */
	Bool save_unders;
	long root_input_mask;	/* initial root input mask */
} Screen;

/*
 * Format structure; describes ZFormat data the screen will understand.
 */
typedef struct {
	XExtData *ext_data;	/* hook for extension to hang data */
	int depth;		/* depth of this image format */
	int bits_per_pixel;	/* bits/pixel at this depth */
	int scanline_pad;	/* scanline must padded to this multiple */
} ScreenFormat;

/*
 * Data structure for setting window attributes.
 */
typedef struct {
    Pixmap background_pixmap;	/* background or None or ParentRelative */
    unsigned long background_pixel;	/* background pixel */
    Pixmap border_pixmap;	/* border of the window */
    unsigned long border_pixel;	/* border pixel value */
    int bit_gravity;		/* one of bit gravity values */
    int win_gravity;		/* one of the window gravity values */
    int backing_store;		/* NotUseful, WhenMapped, Always */
    unsigned long backing_planes;/* planes to be preserved if possible */
    unsigned long backing_pixel;/* value to use in restoring planes */
    Bool save_under;		/* should bits under be saved? (popups) */
    long event_mask;		/* set of events that should be saved */
    long do_not_propagate_mask;	/* set of events that should not propagate */
    Bool override_redirect;	/* boolean value for override-redirect */
    Colormap colormap;		/* color map to be associated with window */
    Cursor cursor;		/* cursor to be displayed (or None) */
} XSetWindowAttributes;

typedef struct {
    int x, y;			/* location of window */
    int width, height;		/* width and height of window */
    int border_width;		/* border width of window */
    int depth;          	/* depth of window */
    Visual *visual;		/* the associated visual structure */
    Window root;        	/* root of screen containing window */
#if defined(__cplusplus) || defined(c_plusplus)
    int c_class;		/* C++ InputOutput, InputOnly*/
#else
    int class;			/* InputOutput, InputOnly*/
#endif
    int bit_gravity;		/* one of bit gravity values */
    int win_gravity;		/* one of the window gravity values */
    int backing_store;		/* NotUseful, WhenMapped, Always */
    unsigned long backing_planes;/* planes to be preserved if possible */
    unsigned long backing_pixel;/* value to be used when restoring planes */
    Bool save_under;		/* boolean, should bits under be saved? */
    Colormap colormap;		/* color map to be associated with window */
    Bool map_installed;		/* boolean, is color map currently installed*/
    int map_state;		/* IsUnmapped, IsUnviewable, IsViewable */
    long all_event_masks;	/* set of events all people have interest in*/
    long your_event_mask;	/* my event mask */
    long do_not_propagate_mask; /* set of events that should not propagate */
    Bool override_redirect;	/* boolean value for override-redirect */
    Screen *screen;		/* back pointer to correct screen */
} XWindowAttributes;

/*
 * Data structure for "image" data, used by image manipulation routines.
 */
typedef struct _XImage {
    int width, height;		/* size of image */
    int xoffset;		/* number of pixels offset in X direction */
    int format;			/* XYBitmap, XYPixmap, ZPixmap */
    char *data;			/* pointer to image data */
    int byte_order;		/* data byte order, LSBFirst, MSBFirst */
    int bitmap_unit;		/* quant. of scanline 8, 16, 32 */
    int bitmap_bit_order;	/* LSBFirst, MSBFirst */
    int bitmap_pad;		/* 8, 16, 32 either XY or ZPixmap */
    int depth;			/* depth of image */
    int bytes_per_line;		/* accelerator to next line */
    int bits_per_pixel;		/* bits per pixel (ZPixmap) */
    unsigned long red_mask;	/* bits in z arrangement */
    unsigned long green_mask;
    unsigned long blue_mask;
    XPointer obdata;		/* hook for the object routines to hang on */
    struct funcs {		/* image manipulation routines */
	struct _XImage *(*create_image)(
		struct _XDisplay* /* display */,
		Visual*		/* visual */,
		unsigned int	/* depth */,
		int		/* format */,
		int		/* offset */,
		char*		/* data */,
		unsigned int	/* width */,
		unsigned int	/* height */,
		int		/* bitmap_pad */,
		int		/* bytes_per_line */);
	int (*destroy_image)        (struct _XImage *);
	unsigned long (*get_pixel)  (struct _XImage *, int, int);
	int (*put_pixel)            (struct _XImage *, int, int, unsigned long);
	struct _XImage *(*sub_image)(struct _XImage *, int, int, unsigned int, unsigned int);
	int (*add_pixel)            (struct _XImage *, long);
	} f;
} XImage;

/*
 * Data structure for XReconfigureWindow
 */
typedef struct {
    int x, y;
    int width, height;
    int border_width;
    Window sibling;
    int stack_mode;
} XWindowChanges;

/*
 * Data structure used by color operations
 */
typedef struct {
	unsigned long pixel;
	unsigned short red, green, blue;
	char flags;  /* do_red, do_green, do_blue */
	char pad;
} XColor;

/*
 * Data structures for graphics operations.  On most machines, these are
 * congruent with the wire protocol structures, so reformatting the data
 * can be avoided on these architectures.
 */
typedef struct {
    short x, y;
    unsigned short width, height;
} XRectangle;


/*
 * Display datatype maintaining display specific data.
 * The contents of this structure are implementation dependent.
 * A Display should be treated as opaque by application code.
 */
#ifndef XLIB_ILLEGAL_ACCESS
typedef struct _XDisplay Display;
#endif

struct _XPrivate;		/* Forward declare before use for C++ */
struct _XrmHashBucketRec;

typedef struct
#ifdef XLIB_ILLEGAL_ACCESS
_XDisplay
#endif
{
	XExtData *ext_data;	/* hook for extension to hang data */
	struct _XPrivate *private1;
	int fd;			/* Network socket. */
	int private2;
	int proto_major_version;/* major version of server's X protocol */
	int proto_minor_version;/* minor version of servers X protocol */
	char *vendor;		/* vendor of the server hardware */
        XID private3;
	XID private4;
	XID private5;
	int private6;
	XID (*resource_alloc)(	/* allocator function */
		struct _XDisplay*
	);
	int byte_order;		/* screen byte order, LSBFirst, MSBFirst */
	int bitmap_unit;	/* padding and data requirements */
	int bitmap_pad;		/* padding requirements on bitmaps */
	int bitmap_bit_order;	/* LeastSignificant or MostSignificant */
	int nformats;		/* number of pixmap formats in list */
	ScreenFormat *pixmap_format;	/* pixmap format list */
	int private8;
	int release;		/* release of the server */
	struct _XPrivate *private9, *private10;
	int qlen;		/* Length of input event queue */
	unsigned long last_request_read; /* seq number of last event read */
	unsigned long request;	/* sequence number of last request. */
	XPointer private11;
	XPointer private12;
	XPointer private13;
	XPointer private14;
	unsigned max_request_size; /* maximum number 32 bit words in request*/
	struct _XrmHashBucketRec *db;
	int (*private15)(
		struct _XDisplay*
		);
	char *display_name;	/* "host:display" string used on this connect*/
	int default_screen;	/* default screen for operations */
	int nscreens;		/* number of screens on this server*/
	Screen *screens;	/* pointer to list of screens */
	unsigned long motion_buffer;	/* size of motion buffer */
	unsigned long private16;
	int min_keycode;	/* minimum defined keycode */
	int max_keycode;	/* maximum defined keycode */
	XPointer private17;
	XPointer private18;
	int private19;
	char *xdefaults;	/* contents of defaults from server */
	/* there is more to this structure, but it is private to Xlib */
}
#ifdef XLIB_ILLEGAL_ACCESS
Display,
#endif
*_XPrivDisplay;

#undef _XEVENT_
#ifndef _XEVENT_
/*
 * Definitions of specific events.
 */
typedef struct {
	int type;		/* of event */
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;	        /* "event" window it is reported relative to */
	Window root;	        /* root window that the event occurred on */
	Window subwindow;	/* child window */
	Time time;		/* milliseconds */
	int x, y;		/* pointer x, y coordinates in event window */
	int x_root, y_root;	/* coordinates relative to root */
	unsigned int state;	/* key or button mask */
	unsigned int keycode;	/* detail */
	Bool same_screen;	/* same screen flag */
} XKeyEvent;
typedef XKeyEvent XKeyPressedEvent;
typedef XKeyEvent XKeyReleasedEvent;

typedef struct {
	int type;		/* of event */
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;	        /* "event" window it is reported relative to */
	Window root;	        /* root window that the event occurred on */
	Window subwindow;	/* child window */
	Time time;		/* milliseconds */
	int x, y;		/* pointer x, y coordinates in event window */
	int x_root, y_root;	/* coordinates relative to root */
	unsigned int state;	/* key or button mask */
	unsigned int button;	/* detail */
	Bool same_screen;	/* same screen flag */
} XButtonEvent;
typedef XButtonEvent XButtonPressedEvent;
typedef XButtonEvent XButtonReleasedEvent;

typedef struct {
	int type;		/* of event */
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;	        /* "event" window reported relative to */
	Window root;	        /* root window that the event occurred on */
	Window subwindow;	/* child window */
	Time time;		/* milliseconds */
	int x, y;		/* pointer x, y coordinates in event window */
	int x_root, y_root;	/* coordinates relative to root */
	unsigned int state;	/* key or button mask */
	char is_hint;		/* detail */
	Bool same_screen;	/* same screen flag */
} XMotionEvent;
typedef XMotionEvent XPointerMovedEvent;

typedef struct {
	int type;		/* of event */
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;	        /* "event" window reported relative to */
	Window root;	        /* root window that the event occurred on */
	Window subwindow;	/* child window */
	Time time;		/* milliseconds */
	int x, y;		/* pointer x, y coordinates in event window */
	int x_root, y_root;	/* coordinates relative to root */
	int mode;		/* NotifyNormal, NotifyGrab, NotifyUngrab */
	int detail;
	/*
	 * NotifyAncestor, NotifyVirtual, NotifyInferior,
	 * NotifyNonlinear,NotifyNonlinearVirtual
	 */
	Bool same_screen;	/* same screen flag */
	Bool focus;		/* boolean focus */
	unsigned int state;	/* key or button mask */
} XCrossingEvent;
typedef XCrossingEvent XEnterWindowEvent;
typedef XCrossingEvent XLeaveWindowEvent;

typedef struct {
	int type;		/* FocusIn or FocusOut */
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;		/* window of event */
	int mode;		/* NotifyNormal, NotifyWhileGrabbed,
				   NotifyGrab, NotifyUngrab */
	int detail;
	/*
	 * NotifyAncestor, NotifyVirtual, NotifyInferior,
	 * NotifyNonlinear,NotifyNonlinearVirtual, NotifyPointer,
	 * NotifyPointerRoot, NotifyDetailNone
	 */
} XFocusChangeEvent;
typedef XFocusChangeEvent XFocusInEvent;
typedef XFocusChangeEvent XFocusOutEvent;

/* generated on EnterWindow and FocusIn  when KeyMapState selected */
typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	char key_vector[32];
} XKeymapEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	int x, y;
	int width, height;
	int count;		/* if non-zero, at least this many more */
} XExposeEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Drawable drawable;
	int x, y;
	int width, height;
	int count;		/* if non-zero, at least this many more */
	int major_code;		/* core is CopyArea or CopyPlane */
	int minor_code;		/* not defined in the core */
} XGraphicsExposeEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Drawable drawable;
	int major_code;		/* core is CopyArea or CopyPlane */
	int minor_code;		/* not defined in the core */
} XNoExposeEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	int state;		/* Visibility state */
} XVisibilityEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window parent;		/* parent of the window */
	Window window;		/* window id of window created */
	int x, y;		/* window location */
	int width, height;	/* size of window */
	int border_width;	/* border width */
	Bool override_redirect;	/* creation should be overridden */
} XCreateWindowEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window event;
	Window window;
} XDestroyWindowEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window event;
	Window window;
	Bool from_configure;
} XUnmapEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window event;
	Window window;
	Bool override_redirect;	/* boolean, is override set... */
} XMapEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window parent;
	Window window;
} XMapRequestEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window event;
	Window window;
	Window parent;
	int x, y;
	Bool override_redirect;
} XReparentEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window event;
	Window window;
	int x, y;
	int width, height;
	int border_width;
	Window above;
	Bool override_redirect;
} XConfigureEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window event;
	Window window;
	int x, y;
} XGravityEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	int width, height;
} XResizeRequestEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window parent;
	Window window;
	int x, y;
	int width, height;
	int border_width;
	Window above;
	int detail;		/* Above, Below, TopIf, BottomIf, Opposite */
	unsigned long value_mask;
} XConfigureRequestEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window event;
	Window window;
	int place;		/* PlaceOnTop, PlaceOnBottom */
} XCirculateEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window parent;
	Window window;
	int place;		/* PlaceOnTop, PlaceOnBottom */
} XCirculateRequestEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	Atom atom;
	Time time;
	int state;		/* NewValue, Deleted */
} XPropertyEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	Atom selection;
	Time time;
} XSelectionClearEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window owner;
	Window requestor;
	Atom selection;
	Atom target;
	Atom property;
	Time time;
} XSelectionRequestEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window requestor;
	Atom selection;
	Atom target;
	Atom property;		/* ATOM or None */
	Time time;
} XSelectionEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	Colormap colormap;	/* COLORMAP or None */
#if defined(__cplusplus) || defined(c_plusplus)
	Bool c_new;		/* C++ */
#else
	Bool new;
#endif
	int state;		/* ColormapInstalled, ColormapUninstalled */
} XColormapEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;
	Atom message_type;
	int format;
	union {
		char b[20];
		short s[10];
		long l[5];
		} data;
} XClientMessageEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;	/* Display the event was read from */
	Window window;		/* unused */
	int request;		/* one of MappingModifier, MappingKeyboard,
				   MappingPointer */
	int first_keycode;	/* first keycode */
	int count;		/* defines range of change w. first_keycode*/
} XMappingEvent;

typedef struct {
	int type;
	Display *display;	/* Display the event was read from */
	XID resourceid;		/* resource id */
	unsigned long serial;	/* serial number of failed request */
	unsigned char error_code;	/* error code of failed request */
	unsigned char request_code;	/* Major op-code of failed request */
	unsigned char minor_code;	/* Minor op-code of failed request */
} XErrorEvent;

typedef struct {
	int type;
	unsigned long serial;	/* # of last request processed by server */
	Bool send_event;	/* true if this came from a SendEvent request */
	Display *display;/* Display the event was read from */
	Window window;	/* window on which event was requested in event mask */
} XAnyEvent;


/***************************************************************
 *
 * GenericEvent.  This event is the standard event for all newer extensions.
 */

typedef struct
    {
    int            type;         /* of event. Always GenericEvent */
    unsigned long  serial;       /* # of last request processed */
    Bool           send_event;   /* true if from SendEvent request */
    Display        *display;     /* Display the event was read from */
    int            extension;    /* major opcode of extension that caused the event */
    int            evtype;       /* actual event type. */
    } XGenericEvent;

typedef struct {
    int            type;         /* of event. Always GenericEvent */
    unsigned long  serial;       /* # of last request processed */
    Bool           send_event;   /* true if from SendEvent request */
    Display        *display;     /* Display the event was read from */
    int            extension;    /* major opcode of extension that caused the event */
    int            evtype;       /* actual event type. */
    unsigned int   cookie;
    void           *data;
} XGenericEventCookie;

/*
 * this union is defined so Xlib can always use the same sized
 * event structure internally, to avoid memory fragmentation.
 */
typedef union _XEvent {
        int type;		/* must not be changed; first element */
	XAnyEvent xany;
	XKeyEvent xkey;
	XButtonEvent xbutton;
	XMotionEvent xmotion;
	XCrossingEvent xcrossing;
	XFocusChangeEvent xfocus;
	XExposeEvent xexpose;
	XGraphicsExposeEvent xgraphicsexpose;
	XNoExposeEvent xnoexpose;
	XVisibilityEvent xvisibility;
	XCreateWindowEvent xcreatewindow;
	XDestroyWindowEvent xdestroywindow;
	XUnmapEvent xunmap;
	XMapEvent xmap;
	XMapRequestEvent xmaprequest;
	XReparentEvent xreparent;
	XConfigureEvent xconfigure;
	XGravityEvent xgravity;
	XResizeRequestEvent xresizerequest;
	XConfigureRequestEvent xconfigurerequest;
	XCirculateEvent xcirculate;
	XCirculateRequestEvent xcirculaterequest;
	XPropertyEvent xproperty;
	XSelectionClearEvent xselectionclear;
	XSelectionRequestEvent xselectionrequest;
	XSelectionEvent xselection;
	XColormapEvent xcolormap;
	XClientMessageEvent xclient;
	XMappingEvent xmapping;
	XErrorEvent xerror;
	XKeymapEvent xkeymap;
	XGenericEvent xgeneric;
	XGenericEventCookie xcookie;
	long pad[24];
} XEvent;
#endif

#define XAllocID(dpy) ((*((_XPrivDisplay)(dpy))->resource_alloc)((dpy)))

/*
 * per character font metric information.
 */
typedef struct {
    short	lbearing;	/* origin to left edge of raster */
    short	rbearing;	/* origin to right edge of raster */
    short	width;		/* advance to next char's origin */
    short	ascent;		/* baseline to top edge of raster */
    short	descent;	/* baseline to bottom edge of raster */
    unsigned short attributes;	/* per char flags (not predefined) */
} XCharStruct;

/*
 * To allow arbitrary information with fonts, there are additional properties
 * returned.
 */
typedef struct {
    Atom name;
    unsigned long card32;
} XFontProp;

typedef struct {
    XExtData	*ext_data;	/* hook for extension to hang data */
    Font        fid;            /* Font id for this font */
    unsigned	direction;	/* hint about direction the font is painted */
    unsigned	min_char_or_byte2;/* first character */
    unsigned	max_char_or_byte2;/* last character */
    unsigned	min_byte1;	/* first row that exists */
    unsigned	max_byte1;	/* last row that exists */
    Bool	all_chars_exist;/* flag if all characters have non-zero size*/
    unsigned	default_char;	/* char to print for undefined character */
    int         n_properties;   /* how many properties there are */
    XFontProp	*properties;	/* pointer to array of additional properties*/
    XCharStruct	min_bounds;	/* minimum bounds over all existing char*/
    XCharStruct	max_bounds;	/* maximum bounds over all existing char*/
    XCharStruct	*per_char;	/* first_char to last_char information */
    int		ascent;		/* log. extent above baseline for spacing */
    int		descent;	/* log. descent below baseline for spacing */
} XFontStruct;

/* unused:
typedef void (*XOMProc)();
 */

typedef struct _XOM *XOM;
typedef struct _XOC *XOC, *XFontSet;

typedef struct _XIM *XIM;
typedef struct _XIC *XIC;

#define XIMPreeditArea		0x0001L
#define XIMPreeditCallbacks	0x0002L
#define XIMPreeditPosition	0x0004L
#define XIMPreeditNothing	0x0008L
#define XIMPreeditNone		0x0010L
#define XIMStatusArea		0x0100L
#define XIMStatusCallbacks	0x0200L
#define XIMStatusNothing	0x0400L
#define XIMStatusNone		0x0800L

#define XNClientWindow "clientWindow"
#define XNInputStyle "inputStyle"

#define XBufferOverflow		-1
#define XLookupNone		1
#define XLookupChars		2
#define XLookupKeySym		3
#define XLookupBoth		4

_XFUNCPROTOBEGIN

#if defined(WIN32) && !defined(_XLIBINT_)
#define _Xdebug (*_Xdebug_p)
#endif

extern int _Xdebug;

extern XFontStruct *XQueryFont(
    Display*		/* display */,
    XID			/* font_ID */
);

extern XImage *XCreateImage(
    Display*		/* display */,
    Visual*		/* visual */,
    unsigned int	/* depth */,
    int			/* format */,
    int			/* offset */,
    char*		/* data */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    int			/* bitmap_pad */,
    int			/* bytes_per_line */
);

/*
 * X function declarations.
 */
extern Display *XOpenDisplay(
    const char*	/* display_name */
);

extern char *XDisplayName(
    const char*	/* string */
);
extern char *XKeysymToString(
    KeySym		/* keysym */
);

extern Atom XInternAtom(
    Display*		/* display */,
    const char*	/* atom_name */,
    Bool		/* only_if_exists */
);
extern Status XInternAtoms(
    Display*		/* dpy */,
    char**		/* names */,
    int			/* count */,
    Bool		/* onlyIfExists */,
    Atom*		/* atoms_return */
);
extern Colormap XCreateColormap(
    Display*		/* display */,
    Window		/* w */,
    Visual*		/* visual */,
    int			/* alloc */
);
extern Cursor XCreatePixmapCursor(
    Display*		/* display */,
    Pixmap		/* source */,
    Pixmap		/* mask */,
    XColor*		/* foreground_color */,
    XColor*		/* background_color */,
    unsigned int	/* x */,
    unsigned int	/* y */
);
extern Cursor XCreateFontCursor(
    Display*		/* display */,
    unsigned int	/* shape */
);
extern Font XLoadFont(
    Display*		/* display */,
    const char*	/* name */
);
extern GC XCreateGC(
    Display*		/* display */,
    Drawable		/* d */,
    unsigned long	/* valuemask */,
    void*			/* values */
);
extern GContext XGContextFromGC(
    GC			/* gc */
);
extern void XFlushGC(
    Display*		/* display */,
    GC			/* gc */
);
extern Pixmap XCreatePixmap(
    Display*		/* display */,
    Drawable		/* d */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    unsigned int	/* depth */
);
extern Pixmap XCreateBitmapFromData(
    Display*		/* display */,
    Drawable		/* d */,
    const char*		/* data */,
    unsigned int	/* width */,
    unsigned int	/* height */
);
extern Pixmap XCreatePixmapFromBitmapData(
    Display*		/* display */,
    Drawable		/* d */,
    char*		/* data */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    unsigned long	/* fg */,
    unsigned long	/* bg */,
    unsigned int	/* depth */
);
extern Window XCreateSimpleWindow(
    Display*		/* display */,
    Window		/* parent */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    unsigned int	/* border_width */,
    unsigned long	/* border */,
    unsigned long	/* background */
);
extern Window XGetSelectionOwner(
    Display*		/* display */,
    Atom		/* selection */
);
extern Window XCreateWindow(
    Display*		/* display */,
    Window		/* parent */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    unsigned int	/* border_width */,
    int			/* depth */,
    unsigned int	/* class */,
    Visual*		/* visual */,
    unsigned long	/* valuemask */,
    XSetWindowAttributes*	/* attributes */
);
extern KeySym XLookupKeysym(
    XKeyEvent*		/* key_event */,
    int			/* index */
);
extern VisualID XVisualIDFromVisual(
    Visual*		/* visual */
);

/* multithread routines */

extern Status XInitThreads(
    void
);

extern void XLockDisplay(
    Display*		/* display */
);

extern void XUnlockDisplay(
    Display*		/* display */
);

/* these are routines for which there are also macros */
extern Window XRootWindow(
    Display*		/* display */,
    int			/* screen_number */
);
extern Window XDefaultRootWindow(
    Display*		/* display */
);
extern Window XRootWindowOfScreen(
    Screen*		/* screen */
);
extern Visual *XDefaultVisual(
    Display*		/* display */,
    int			/* screen_number */
);
extern Visual *XDefaultVisualOfScreen(
    Screen*		/* screen */
);
extern GC XDefaultGC(
    Display*		/* display */,
    int			/* screen_number */
);
extern GC XDefaultGCOfScreen(
    Screen*		/* screen */
);
extern unsigned long XBlackPixel(
    Display*		/* display */,
    int			/* screen_number */
);
extern unsigned long XWhitePixel(
    Display*		/* display */,
    int			/* screen_number */
);
extern unsigned long XBlackPixelOfScreen(
    Screen*		/* screen */
);
extern unsigned long XWhitePixelOfScreen(
    Screen*		/* screen */
);
extern char *XDisplayString(
    Display*		/* display */
);
extern Colormap XDefaultColormap(
    Display*		/* display */,
    int			/* screen_number */
);
extern Colormap XDefaultColormapOfScreen(
    Screen*		/* screen */
);
extern Display *XDisplayOfScreen(
    Screen*		/* screen */
);
extern Screen *XScreenOfDisplay(
    Display*		/* display */,
    int			/* screen_number */
);
extern Screen *XDefaultScreenOfDisplay(
    Display*		/* display */
);

typedef int (*XErrorHandler) (	    /* WARNING, this type not in Xlib spec */
    Display*		/* display */,
    XErrorEvent*	/* error_event */
);

extern XErrorHandler XSetErrorHandler (
    XErrorHandler	/* handler */
);


typedef int (*XIOErrorHandler) (    /* WARNING, this type not in Xlib spec */
    Display*		/* display */
);

extern XIOErrorHandler XSetIOErrorHandler (
    XIOErrorHandler	/* handler */
);

typedef void (*XIOErrorExitHandler) ( /* WARNING, this type not in Xlib spec */
    Display*,		/* display */
    void*		/* user_data */
);

extern void XSetIOErrorExitHandler (
    Display*,			/* display */
    XIOErrorExitHandler,	/* handler */
    void*			/* user_data */
);

/* ICCCM routines for things that don't require special include files; */
/* other declarations are given in Xutil.h                             */
extern Status XSetWMProtocols(
    Display*		/* display */,
    Window		/* w */,
    Atom*		/* protocols */,
    int			/* count */
);
extern Status XIconifyWindow(
    Display*		/* display */,
    Window		/* w */,
    int			/* screen_number */
);

extern int XSetTransientForHint(
    Display*		/* display */,
    Window		/* w */,
    Window		/* prop_window */
);

/* The following are given in alphabetical order */
extern Status XAllocColor(
    Display*		/* display */,
    Colormap		/* colormap */,
    XColor*		/* screen_in_out */
);

extern int XChangeProperty(
    Display*		/* display */,
    Window		/* w */,
    Atom		/* property */,
    Atom		/* type */,
    int			/* format */,
    int			/* mode */,
    const unsigned char*	/* data */,
    int			/* nelements */
);

extern int XChangeWindowAttributes(
    Display*		/* display */,
    Window		/* w */,
    unsigned long	/* valuemask */,
    XSetWindowAttributes* /* attributes */
);

extern Status XGetWindowAttributes(
    Display*		/* display */,
    Window		/* w */,
    XWindowAttributes*	/* window_attributes_return */
);

extern Bool XCheckIfEvent(
    Display*		/* display */,
    XEvent*		/* event_return */,
    Bool (*) (
	       Display*			/* display */,
               XEvent*			/* event */,
               XPointer			/* arg */
             )		/* predicate */,
    XPointer		/* arg */
);

extern int XClearArea(
    Display*		/* display */,
    Window		/* w */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    Bool		/* exposures */
);

extern int XClearWindow(
    Display*		/* display */,
    Window		/* w */
);

extern int XCloseDisplay(
    Display*		/* display */
);

extern int XConfigureWindow(
    Display*		/* display */,
    Window		/* w */,
    unsigned int	/* value_mask */,
    XWindowChanges*	/* values */
);

extern int XConvertSelection(
    Display*		/* display */,
    Atom		/* selection */,
    Atom 		/* target */,
    Atom		/* property */,
    Window		/* requestor */,
    Time		/* time */
);

extern int XCopyArea(
    Display*		/* display */,
    Drawable		/* src */,
    Drawable		/* dest */,
    GC			/* gc */,
    int			/* src_x */,
    int			/* src_y */,
    unsigned int	/* width */,
    unsigned int	/* height */,
    int			/* dest_x */,
    int			/* dest_y */
);

extern int XDefaultDepth(
    Display*		/* display */,
    int			/* screen_number */
);

extern int XDefaultDepthOfScreen(
    Screen*		/* screen */
);

extern int XDefaultScreen(
    Display*		/* display */
);

extern int XDefineCursor(
    Display*		/* display */,
    Window		/* w */,
    Cursor		/* cursor */
);

extern int XDeleteProperty(
    Display*		/* display */,
    Window		/* w */,
    Atom		/* property */
);

extern int XDestroyWindow(
    Display*		/* display */,
    Window		/* w */
);

extern int XDisplayHeight(
    Display*		/* display */,
    int			/* screen_number */
);

extern int XDisplayWidth(
    Display*		/* display */,
    int			/* screen_number */
);

extern int XDrawLine(
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x1 */,
    int			/* y1 */,
    int			/* x2 */,
    int			/* y2 */
);

extern int XDrawRectangle(
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */
);

extern int XDrawString(
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    int			/* x */,
    int			/* y */,
    const char*	/* string */,
    int			/* length */
);

extern int XEventsQueued(
    Display*		/* display */,
    int			/* mode */
);

extern int XFlush(
    Display*		/* display */
);

extern int XFree(
    void*		/* data */
);

extern int XFreeColormap(
    Display*		/* display */,
    Colormap		/* colormap */
);

extern int XFreeColors(
    Display*		/* display */,
    Colormap		/* colormap */,
    unsigned long*	/* pixels */,
    int			/* npixels */,
    unsigned long	/* planes */
);

extern int XFreeCursor(
    Display*		/* display */,
    Cursor		/* cursor */
);

extern int XFreeFont(
    Display*		/* display */,
    XFontStruct*	/* font_struct */
);

extern int XFreeFontInfo(
    char**		/* names */,
    XFontStruct*	/* free_info */,
    int			/* actual_count */
);

extern int XFreeFontNames(
    char**		/* list */
);

extern int XFreeFontPath(
    char**		/* list */
);

extern int XFreeGC(
    Display*		/* display */,
    GC			/* gc */
);

extern int XFreePixmap(
    Display*		/* display */,
    Pixmap		/* pixmap */
);

extern int XGetInputFocus(
    Display*		/* display */,
    Window*		/* focus_return */,
    int*		/* revert_to_return */
);

extern Status XGetTransientForHint(
    Display*		/* display */,
    Window		/* w */,
    Window*		/* prop_window_return */
);

extern int XGetWindowProperty(
    Display*		/* display */,
    Window		/* w */,
    Atom		/* property */,
    long		/* long_offset */,
    long		/* long_length */,
    Bool		/* delete */,
    Atom		/* req_type */,
    Atom*		/* actual_type_return */,
    int*		/* actual_format_return */,
    unsigned long*	/* nitems_return */,
    unsigned long*	/* bytes_after_return */,
    unsigned char**	/* prop_return */
);

extern int XGrabButton(
    Display*		/* display */,
    unsigned int	/* button */,
    unsigned int	/* modifiers */,
    Window		/* grab_window */,
    Bool		/* owner_events */,
    unsigned int	/* event_mask */,
    int			/* pointer_mode */,
    int			/* keyboard_mode */,
    Window		/* confine_to */,
    Cursor		/* cursor */
);

extern int XGrabKey(
    Display*		/* display */,
    int			/* keycode */,
    unsigned int	/* modifiers */,
    Window		/* grab_window */,
    Bool		/* owner_events */,
    int			/* pointer_mode */,
    int			/* keyboard_mode */
);

extern int XGrabKeyboard(
    Display*		/* display */,
    Window		/* grab_window */,
    Bool		/* owner_events */,
    int			/* pointer_mode */,
    int			/* keyboard_mode */,
    Time		/* time */
);

extern int XGrabPointer(
    Display*		/* display */,
    Window		/* grab_window */,
    Bool		/* owner_events */,
    unsigned int	/* event_mask */,
    int			/* pointer_mode */,
    int			/* keyboard_mode */,
    Window		/* confine_to */,
    Cursor		/* cursor */,
    Time		/* time */
);

extern int XGrabServer(
    Display*		/* display */
);

extern int XHeightOfScreen(
    Screen*		/* screen */
);

extern int XIfEvent(
    Display*		/* display */,
    XEvent*		/* event_return */,
    Bool (*) (
	       Display*			/* display */,
               XEvent*			/* event */,
               XPointer			/* arg */
             )		/* predicate */,
    XPointer		/* arg */
);

extern KeyCode XKeysymToKeycode(
    Display*		/* display */,
    KeySym		/* keysym */
);

extern int XLowerWindow(
    Display*		/* display */,
    Window		/* w */
);

extern int XMapRaised(
    Display*		/* display */,
    Window		/* w */
);

extern int XMapSubwindows(
    Display*		/* display */,
    Window		/* w */
);

extern int XMapWindow(
    Display*		/* display */,
    Window		/* w */
);

extern int XMaskEvent(
    Display*		/* display */,
    long		/* event_mask */,
    XEvent*		/* event_return */
);

extern int XMoveResizeWindow(
    Display*		/* display */,
    Window		/* w */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */
);

extern int XMoveWindow(
    Display*		/* display */,
    Window		/* w */,
    int			/* x */,
    int			/* y */
);

extern int XNextEvent(
    Display*		/* display */,
    XEvent*		/* event_return */
);

extern int XPeekEvent(
    Display*		/* display */,
    XEvent*		/* event_return */
);

extern int XPeekIfEvent(
    Display*		/* display */,
    XEvent*		/* event_return */,
    Bool (*) (
	       Display*		/* display */,
               XEvent*		/* event */,
               XPointer		/* arg */
             )		/* predicate */,
    XPointer		/* arg */
);

extern int XPending(
    Display*		/* display */
);


extern int XPutImage(
    Display*		/* display */,
    Drawable		/* d */,
    GC			/* gc */,
    XImage*		/* image */,
    int			/* src_x */,
    int			/* src_y */,
    int			/* dest_x */,
    int			/* dest_y */,
    unsigned int	/* width */,
    unsigned int	/* height */
);

extern Bool XQueryExtension(
    Display*		/* display */,
    const char*	/* name */,
    int*		/* major_opcode_return */,
    int*		/* first_event_return */,
    int*		/* first_error_return */
);

extern Bool XQueryPointer(
    Display*		/* display */,
    Window		/* w */,
    Window*		/* root_return */,
    Window*		/* child_return */,
    int*		/* root_x_return */,
    int*		/* root_y_return */,
    int*		/* win_x_return */,
    int*		/* win_y_return */,
    unsigned int*       /* mask_return */
);

extern int XRaiseWindow(
    Display*		/* display */,
    Window		/* w */
);

extern int XRefreshKeyboardMapping(
    XMappingEvent*	/* event_map */
);

extern int XReparentWindow(
    Display*		/* display */,
    Window		/* w */,
    Window		/* parent */,
    int			/* x */,
    int			/* y */
);

extern int XResizeWindow(
    Display*		/* display */,
    Window		/* w */,
    unsigned int	/* width */,
    unsigned int	/* height */
);

extern int XScreenCount(
    Display*		/* display */
);

extern int XSelectInput(
    Display*		/* display */,
    Window		/* w */,
    long		/* event_mask */
);

extern Status XSendEvent(
    Display*		/* display */,
    Window		/* w */,
    Bool		/* propagate */,
    long		/* event_mask */,
    XEvent*		/* event_send */
);

extern int XSetBackground(
    Display*		/* display */,
    GC			/* gc */,
    unsigned long	/* background */
);

extern int XSetForeground(
    Display*		/* display */,
    GC			/* gc */,
    unsigned long	/* foreground */
);

extern int XSetSelectionOwner(
    Display*		/* display */,
    Atom	        /* selection */,
    Window		/* owner */,
    Time		/* time */
);

extern int XSetWindowBackground(
    Display*		/* display */,
    Window		/* w */,
    unsigned long	/* background_pixel */
);

extern int XStoreName(
    Display*		/* display */,
    Window		/* w */,
    const char*	/* window_name */
);

extern int XSync(
    Display*		/* display */,
    Bool		/* discard */
);

extern int XTextExtents(
    XFontStruct*	/* font_struct */,
    const char*	/* string */,
    int			/* nchars */,
    int*		/* direction_return */,
    int*		/* font_ascent_return */,
    int*		/* font_descent_return */,
    XCharStruct*	/* overall_return */
);

extern int XTextWidth(
    XFontStruct*	/* font_struct */,
    const char*	/* string */,
    int			/* count */
);

extern int XUndefineCursor(
    Display*		/* display */,
    Window		/* w */
);

extern int XUngrabButton(
    Display*		/* display */,
    unsigned int	/* button */,
    unsigned int	/* modifiers */,
    Window		/* grab_window */
);

extern int XUngrabKey(
    Display*		/* display */,
    int			/* keycode */,
    unsigned int	/* modifiers */,
    Window		/* grab_window */
);

extern int XUngrabKeyboard(
    Display*		/* display */,
    Time		/* time */
);

extern int XUngrabPointer(
    Display*		/* display */,
    Time		/* time */
);

extern int XUngrabServer(
    Display*		/* display */
);

extern int XUnmapWindow(
    Display*		/* display */,
    Window		/* w */
);

extern int XWarpPointer(
    Display*		/* display */,
    Window		/* src_w */,
    Window		/* dest_w */,
    int			/* src_x */,
    int			/* src_y */,
    unsigned int	/* src_width */,
    unsigned int	/* src_height */,
    int			/* dest_x */,
    int			/* dest_y */
);

extern int XWidthOfScreen(
    Screen*		/* screen */
);

extern int XWindowEvent(
    Display*		/* display */,
    Window		/* w */,
    long		/* event_mask */,
    XEvent*		/* event_return */
);

extern XIM XOpenIM(
    Display*			/* dpy */,
    struct _XrmHashBucketRec*	/* rdb */,
    char*			/* res_name */,
    char*			/* res_class */
);

extern Status XCloseIM(
    XIM /* im */
);

extern XIC XCreateIC(
    XIM /* im */, ...
);

extern void XDestroyIC(
    XIC /* ic */
);

extern Bool XFilterEvent(
    XEvent*	/* event */,
    Window	/* window */
);

extern int Xutf8LookupString(
    XIC			/* ic */,
    XKeyPressedEvent*	/* event */,
    char*		/* buffer_return */,
    int			/* bytes_buffer */,
    KeySym*		/* keysym_return */,
    Status*		/* status_return */
);

extern Bool XGetEventData(
    Display*			/* dpy */,
    XGenericEventCookie*	/* cookie*/
);

extern void XFreeEventData(
    Display*			/* dpy */,
    XGenericEventCookie*	/* cookie*/
);

#ifdef __clang__
#pragma clang diagnostic pop
#endif

_XFUNCPROTOEND

#endif /* _X11_XLIB_H_ */
