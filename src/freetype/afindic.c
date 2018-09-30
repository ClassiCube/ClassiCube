/***************************************************************************/
/*                                                                         */
/*  afindic.c                                                              */
/*                                                                         */
/*    Auto-fitter hinting routines for Indic writing system (body).        */
/*                                                                         */
/*  Copyright 2007-2018 by                                                 */
/*  Rahul Bhalerao <rahul.bhalerao@redhat.com>, <b.rahul.pm@gmail.com>.    */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#include "aftypes.h"
#include "aflatin.h"
#include "afcjk.h"

  AF_DEFINE_WRITING_SYSTEM_CLASS(
    af_indic_writing_system_class,

    AF_WRITING_SYSTEM_INDIC,

    sizeof ( AF_CJKMetricsRec ),

    (AF_WritingSystem_InitMetricsFunc) NULL, /* style_metrics_init    */
    (AF_WritingSystem_ScaleMetricsFunc)NULL, /* style_metrics_scale   */
    (AF_WritingSystem_DoneMetricsFunc) NULL, /* style_metrics_done    */
    (AF_WritingSystem_GetStdWidthsFunc)NULL, /* style_metrics_getstdw */

    (AF_WritingSystem_InitHintsFunc)   NULL, /* style_hints_init      */
    (AF_WritingSystem_ApplyHintsFunc)  NULL  /* style_hints_apply     */
  )

/* END */
