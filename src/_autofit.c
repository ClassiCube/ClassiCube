/***************************************************************************/
/*                                                                         */
/*  autofit.c                                                              */
/*                                                                         */
/*    Auto-fitter module (body).                                           */
/*                                                                         */
/*  Copyright 2003-2018 by                                                 */
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

#include "freetype/afangles.c"
#include "freetype/afblue.c"
#include "freetype/afdummy.c"
#include "freetype/afglobal.c"
#include "freetype/afhints.c"
#include "freetype/aflatin.c"
#include "freetype/afloader.c"
#include "freetype/afmodule.c"
#include "freetype/afranges.c"
#include "freetype/afshaper.c"
#include "freetype/afwarp.c"
#endif

/* END */
