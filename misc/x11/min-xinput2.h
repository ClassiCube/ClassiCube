/*
 * Copyright © 2009 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

/* Definitions used by the library and client */

#ifndef _XINPUT2_H_
#define _XINPUT2_H_

/* XI2 event mask macros */
#define XISetMask(ptr, event)   (((unsigned char*)(ptr))[(event)>>3] |=  (1 << ((event) & 7)))
#define XIClearMask(ptr, event) (((unsigned char*)(ptr))[(event)>>3] &= ~(1 << ((event) & 7)))
#define XIMaskIsSet(ptr, event) (((unsigned char*)(ptr))[(event)>>3] &   (1 << ((event) & 7)))
#define XIMaskLen(event)        (((event) >> 3) + 1)

/* Fake device ID's for event selection */
#define XIAllDevices                            0
#define XIAllMasterDevices                      1

/* Event types */
#define XI_RawMotion                     17

#define XI_GestureSwipeEnd               32
#define XI_LASTEVENT                     XI_GestureSwipeEnd
/* NOTE: XI2LASTEVENT in xserver/include/inputstr.h must be the same value
 * as XI_LASTEVENT if the server is supposed to handle masks etc. for this
 * type of event. */


/**
 * Generic Event mask.
 * To be used whenever a list of masks per extension has to be provided.
 *
 * But, don't actually use the CARD{8,16,32} types.  We can't get them them
 * defined here without polluting the namespace.
 */
typedef struct {
    unsigned char       extension;
    unsigned char       pad0;
    unsigned short      pad1;
    unsigned int      evmask;
} XGenericEventMask;

/*******************************************************************
 *
 */
typedef struct {
    int           mask_len;
    unsigned char *mask;
    double        *values;
} XIValuatorState;


typedef struct
{
    int                 deviceid;
    int                 mask_len;
    unsigned char*      mask;
} XIEventMask;

/**
 * Generic XI2 event. All XI2 events have the same header.
 */
typedef struct {
    int           type;         /* GenericEvent */
    unsigned long serial;       /* # of last request processed by server */
    Bool          send_event;   /* true if this came from a SendEvent request */
    Display       *display;     /* Display the event was read from */
    int           extension;    /* XI extension offset */
    int           evtype;
    Time          time;
} XIEvent;

typedef struct {
    int           type;         /* GenericEvent */
    unsigned long serial;       /* # of last request processed by server */
    Bool          send_event;   /* true if this came from a SendEvent request */
    Display       *display;     /* Display the event was read from */
    int           extension;    /* XI extension offset */
    int           evtype;       /* XI_RawKeyPress, XI_RawKeyRelease, etc. */
    Time          time;
    int           deviceid;
    int           sourceid;     /* Bug: Always 0. https://bugs.freedesktop.org//show_bug.cgi?id=34240 */
    int           detail;
    int           flags;
    XIValuatorState valuators;
    double        *raw_values;
} XIRawEvent;

_XFUNCPROTOBEGIN

extern int      XISelectEvents(
     Display*            dpy,
     Window              win,
     XIEventMask         *masks,
     int                 num_masks
);

extern Status XIQueryVersion(
     Display*           dpy,
     int*               major_version_inout,
     int*               minor_version_inout
);

_XFUNCPROTOEND

#endif /* XINPUT2_H */
