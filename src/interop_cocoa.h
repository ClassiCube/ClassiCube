#include "_WindowBase.h"
#include "ExtMath.h"
#include "Funcs.h"
#include "Bitmap.h"
#include "String.h"
#include "Options.h"
#include <Cocoa/Cocoa.h>
#include <ApplicationServices/ApplicationServices.h>

// asynkitty: Fixes implicit function declaration error on line 23 from interop_cocoa.m. Might be a weird hack but I don't know.
int CGDisplayBitsPerPixel(CGDirectDisplayID display);