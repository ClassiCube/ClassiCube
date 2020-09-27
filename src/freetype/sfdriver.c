/***************************************************************************/
/*                                                                         */
/*  sfdriver.c                                                             */
/*                                                                         */
/*    High-level SFNT driver interface (body).                             */
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


#include "ft2build.h"
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_SFNT_H
#include FT_INTERNAL_OBJECTS_H
#include FT_TRUETYPE_IDS_H

#include "sfdriver.h"
#include "ttload.h"
#include "sfobjs.h"

#include "sferrors.h"

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
#include "ttsbit.h"
#endif

#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
#include "ttpost.h"
#endif

#include "ttcmap.h"
#include "ttmtx.h"

#include FT_SERVICE_GLYPH_DICT_H
#include FT_SERVICE_TT_CMAP_H

#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
#include FT_MULTIPLE_MASTERS_H
#include FT_SERVICE_MULTIPLE_MASTERS_H
#endif


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_sfdriver


#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES

  /*
   *  GLYPH DICT SERVICE
   *
   */

  static FT_Error
  sfnt_get_glyph_name( FT_Face     face,
                       FT_UInt     glyph_index,
                       FT_Pointer  buffer,
                       FT_UInt     buffer_max )
  {
    FT_String*  gname;
    FT_Error    error;


    error = tt_face_get_ps_name( (TT_Face)face, glyph_index, &gname );
    if ( !error )
      FT_STRCPYN( buffer, gname, buffer_max );

    return error;
  }


  FT_DEFINE_SERVICE_GLYPHDICTREC(
    sfnt_service_glyph_dict,

    (FT_GlyphDict_GetNameFunc)  sfnt_get_glyph_name     /* get_name   */
  )

#endif /* TT_CONFIG_OPTION_POSTSCRIPT_NAMES */


  /*
   *  TT CMAP INFO
   */
  FT_DEFINE_SERVICE_TTCMAPSREC(
    tt_service_get_cmap_info,

    (TT_CMap_Info_GetFunc)tt_get_cmap_info    /* get_cmap_info */
  )


  /*
   *  SERVICE LIST
   */

#if defined TT_CONFIG_OPTION_POSTSCRIPT_NAMES
  FT_DEFINE_SERVICEDESCREC2(
    sfnt_services,

    FT_SERVICE_ID_GLYPH_DICT,           &sfnt_service_glyph_dict,
    FT_SERVICE_ID_TT_CMAP,              &tt_service_get_cmap_info )
#else
  FT_DEFINE_SERVICEDESCREC2(
    sfnt_services,

    FT_SERVICE_ID_SFNT_TABLE,           &sfnt_service_sfnt_table,
    FT_SERVICE_ID_TT_CMAP,              &tt_service_get_cmap_info )
#endif


  FT_CALLBACK_DEF( FT_Module_Interface )
  sfnt_get_interface( FT_Module    module,
                      const char*  module_interface )
  {
    FT_UNUSED( module );

    return ft_service_list_lookup( sfnt_services, module_interface );
  }


#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
#define PUT_EMBEDDED_BITMAPS( a )  a
#else
#define PUT_EMBEDDED_BITMAPS( a )  NULL
#endif

#ifdef TT_CONFIG_OPTION_POSTSCRIPT_NAMES
#define PUT_PS_NAMES( a )  a
#else
#define PUT_PS_NAMES( a )  NULL
#endif

  FT_DEFINE_SFNT_INTERFACE(
    sfnt_interface,

    tt_face_goto_table,     /* TT_Loader_GotoTableFunc goto_table      */

    sfnt_init_face,         /* TT_Init_Face_Func       init_face       */
    sfnt_load_face,         /* TT_Load_Face_Func       load_face       */
    sfnt_done_face,         /* TT_Done_Face_Func       done_face       */
    sfnt_get_interface,     /* FT_Module_Requester     get_interface   */

    tt_face_load_any,       /* TT_Load_Any_Func        load_any        */

    tt_face_load_head,      /* TT_Load_Table_Func      load_head       */
    tt_face_load_hhea,      /* TT_Load_Metrics_Func    load_hhea       */
    tt_face_load_cmap,      /* TT_Load_Table_Func      load_cmap       */
    tt_face_load_maxp,      /* TT_Load_Table_Func      load_maxp       */
    tt_face_load_os2,       /* TT_Load_Table_Func      load_os2        */
    tt_face_load_post,      /* TT_Load_Table_Func      load_post       */

    tt_face_load_name,      /* TT_Load_Table_Func      load_name       */
    tt_face_free_name,      /* TT_Free_Table_Func      free_name       */

    NULL,                   /* TT_Load_Table_Func      load_kern       */
    NULL,                   /* TT_Load_Table_Func      load_gasp       */
    NULL,                   /* TT_Load_Table_Func      load_init       */

    /* see `ttload.h' */
    PUT_EMBEDDED_BITMAPS( tt_face_load_bhed ),
                            /* TT_Load_Table_Func      load_bhed       */
    PUT_EMBEDDED_BITMAPS( tt_face_load_sbit_image ),
                            /* TT_Load_SBit_Image_Func load_sbit_image */

    /* see `ttpost.h' */
    PUT_PS_NAMES( tt_face_get_ps_name   ),
                            /* TT_Get_PS_Name_Func     get_psname      */
    PUT_PS_NAMES( tt_face_free_ps_names ),
                            /* TT_Free_Table_Func      free_psnames    */

    /* since version 2.1.8 */
    NULL,                   /* TT_Face_GetKerningFunc  get_kerning     */

    /* since version 2.2 */
    tt_face_load_font_dir,  /* TT_Load_Table_Func      load_font_dir   */
    tt_face_load_hmtx,      /* TT_Load_Metrics_Func    load_hmtx       */

    /* see `ttsbit.h' and `sfnt.h' */
    PUT_EMBEDDED_BITMAPS( tt_face_load_sbit ),
                            /* TT_Load_Table_Func      load_eblc       */
    PUT_EMBEDDED_BITMAPS( tt_face_free_sbit ),
                            /* TT_Free_Table_Func      free_eblc       */

    PUT_EMBEDDED_BITMAPS( tt_face_set_sbit_strike     ),
                            /* TT_Set_SBit_Strike_Func set_sbit_strike */
    PUT_EMBEDDED_BITMAPS( tt_face_load_strike_metrics ),
                    /* TT_Load_Strike_Metrics_Func load_strike_metrics */

    tt_face_get_metrics,    /* TT_Get_Metrics_Func     get_metrics     */

    tt_face_get_name        /* TT_Get_Name_Func        get_name        */
  )


  FT_DEFINE_MODULE(
    sfnt_module_class,

    0,  /* not a font driver or renderer */
    sizeof ( FT_ModuleRec ),

    "sfnt",     /* driver name                            */
    0x10000L,   /* driver version 1.0                     */
    0x20000L,   /* driver requires FreeType 2.0 or higher */

    (const void*)&sfnt_interface,  /* module specific interface */

    (FT_Module_Constructor)NULL,               /* module_init   */
    (FT_Module_Destructor) NULL,               /* module_done   */
    (FT_Module_Requester)  sfnt_get_interface  /* get_interface */
  )


/* END */
