/***************************************************************************/
/*                                                                         */
/*  cff.c                                                                  */
/*                                                                         */
/*    FreeType OpenType driver component (body only).                      */
/*                                                                         */
/*  Copyright 1996-2018 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/

#include "Core.h"
#ifdef CC_BUILD_FREETYPE
#define FT_MAKE_OPTION_SINGLE_OBJECT
#include "freetype/ft2build.h"

#include "freetype/cffcmap.c"
#include "freetype/cffdrivr.c"
#include "freetype/cffgload.c"
#include "freetype/cffparse.c"
#include "freetype/cffload.c"
#include "freetype/cffobjs.c"
#endif

/* END */
