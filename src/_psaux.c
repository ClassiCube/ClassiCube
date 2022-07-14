/***************************************************************************/
/*                                                                         */
/*  psaux.c                                                                */
/*                                                                         */
/*    FreeType auxiliary PostScript driver component (body only).          */
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

#include "freetype/psauxmod.c"
#include "freetype/psconv.c"
#include "freetype/psobjs.c"
#include "freetype/t1cmap.c"
#include "freetype/t1decode.c"
#include "freetype/cffdecode.c"

#include "freetype/psarrst.c"
#include "freetype/psblues.c"
#include "freetype/pserror.c"
#include "freetype/psfont.c"
#include "freetype/psft.c"
#include "freetype/pshints.c"
#include "freetype/psintrp.c"
#include "freetype/psread.c"
#include "freetype/psstack.c"
#endif

/* END */
