/***************************************************************************/
/*                                                                         */
/*  ftbase.c                                                               */
/*                                                                         */
/*    Single object library component (body only).                         */
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
#include "freetype/ft2build.h"
#define  FT_MAKE_OPTION_SINGLE_OBJECT

#include "freetype/ftadvanc.c"
#include "freetype/ftcalc.c"
#include "freetype/ftfntfmt.c"
#include "freetype/ftgloadr.c"
#include "freetype/fthash.c"
#include "freetype/ftmac.c"
#include "freetype/ftobjs.c"
#include "freetype/ftoutln.c"
#include "freetype/ftstream.c"
#include "freetype/fttrigon.c"
#include "freetype/ftutil.c"
#endif

/* END */
