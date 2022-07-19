/***************************************************************************/
/*                                                                         */
/*  truetype.c                                                             */
/*                                                                         */
/*    FreeType TrueType driver component (body only).                      */
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

#include "freetype/ttdriver.c"   /* driver interface    */
#include "freetype/ttgload.c"    /* glyph loader        */
#include "freetype/ttinterp.c"
#include "freetype/ttobjs.c"     /* object manager      */
#include "freetype/ttpload.c"    /* tables loader       */
#endif

/* END */
