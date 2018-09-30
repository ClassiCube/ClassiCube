/***************************************************************************/
/*                                                                         */
/*  afcjk.c                                                                */
/*                                                                         */
/*    Auto-fitter hinting routines for CJK writing system (body).          */
/*                                                                         */
/*  Copyright 2006-2018 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/

  /*
   *  The algorithm is based on akito's autohint patch, archived at
   *
   *  https://web.archive.org/web/20051219160454/http://www.kde.gr.jp:80/~akito/patch/freetype2/2.1.7/
   *
   */

#include "ft2build.h"
#include FT_ADVANCES_H
#include FT_INTERNAL_DEBUG_H

#include "afglobal.h"
#include "afpic.h"
#include "aflatin.h"
#include "afcjk.h"

  AF_DEFINE_WRITING_SYSTEM_CLASS(
    af_cjk_writing_system_class,

    AF_WRITING_SYSTEM_CJK,

    sizeof ( AF_CJKMetricsRec ),

    (AF_WritingSystem_InitMetricsFunc) NULL, /* style_metrics_init    */
    (AF_WritingSystem_ScaleMetricsFunc)NULL, /* style_metrics_scale   */
    (AF_WritingSystem_DoneMetricsFunc) NULL, /* style_metrics_done    */
    (AF_WritingSystem_GetStdWidthsFunc)NULL, /* style_metrics_getstdw */

    (AF_WritingSystem_InitHintsFunc)   NULL, /* style_hints_init      */
    (AF_WritingSystem_ApplyHintsFunc)  NULL  /* style_hints_apply     */
  )

/* END */
