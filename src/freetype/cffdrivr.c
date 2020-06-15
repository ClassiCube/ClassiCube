/***************************************************************************/
/*                                                                         */
/*  cffdrivr.c                                                             */
/*                                                                         */
/*    OpenType font driver implementation (body).                          */
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
#include FT_FREETYPE_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_SFNT_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H
#include FT_SERVICE_TT_CMAP_H
#include FT_SERVICE_CFF_TABLE_LOAD_H

#include "cffdrivr.h"
#include "cffgload.h"
#include "cffload.h"
#include "cffcmap.h"
#include "cffparse.h"
#include "cffobjs.h"

#include "cfferrs.h"

#include FT_SERVICE_FONT_FORMAT_H
#include FT_SERVICE_GLYPH_DICT_H
#include FT_DRIVER_H


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_cffdriver


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                          F A C E S                              ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    cff_glyph_load                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A driver method used to load a glyph within a given glyph slot.    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    slot        :: A handle to the target slot object where the glyph  */
  /*                   will be loaded.                                     */
  /*                                                                       */
  /*    size        :: A handle to the source face size at which the glyph */
  /*                   must be scaled, loaded, etc.                        */
  /*                                                                       */
  /*    glyph_index :: The index of the glyph in the font file.            */
  /*                                                                       */
  /*    load_flags  :: A flag indicating what to load for this glyph.  The */
  /*                   FT_LOAD_??? constants can be used to control the    */
  /*                   glyph loading process (e.g., whether the outline    */
  /*                   should be scaled, whether to load bitmaps or not,   */
  /*                   whether to hint the outline, etc).                  */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0 means success.                             */
  /*                                                                       */
  FT_CALLBACK_DEF( FT_Error )
  cff_glyph_load( FT_GlyphSlot  cffslot,      /* CFF_GlyphSlot */
                  FT_Size       cffsize,      /* CFF_Size      */
                  FT_UInt       glyph_index,
                  FT_Int32      load_flags )
  {
    FT_Error       error;
    CFF_GlyphSlot  slot = (CFF_GlyphSlot)cffslot;
    CFF_Size       size = (CFF_Size)cffsize;


    if ( !slot )
      return FT_THROW( Invalid_Slot_Handle );

    FT_TRACE1(( "cff_glyph_load: glyph index %d\n", glyph_index ));

    /* check whether we want a scaled outline or bitmap */
    if ( !size )
      load_flags |= FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING;

    /* reset the size object if necessary */
    if ( load_flags & FT_LOAD_NO_SCALE )
      size = NULL;

    if ( size )
    {
      /* these two objects must have the same parent */
      if ( cffsize->face != cffslot->face )
        return FT_THROW( Invalid_Face_Handle );
    }

    /* now load the glyph outline if necessary */
    error = cff_slot_load( slot, size, glyph_index, load_flags );

    /* force drop-out mode to 2 - irrelevant now */
    /* slot->outline.dropout_mode = 2; */

    return error;
  }


  FT_CALLBACK_DEF( FT_Error )
  cff_get_advances( FT_Face    face,
                    FT_UInt    start,
                    FT_UInt    count,
                    FT_Int32   flags,
                    FT_Fixed*  advances )
  {
    FT_UInt       nn;
    FT_Error      error = FT_Err_Ok;
    FT_GlyphSlot  slot  = face->glyph;


    if ( FT_IS_SFNT( face ) )
    {
      /* OpenType 1.7 mandates that the data from `hmtx' table be used; */
      /* it is no longer necessary that those values are identical to   */
      /* the values in the `CFF' table                                  */

      TT_Face   ttface = (TT_Face)face;
      FT_Short  dummy;


      if ( flags & FT_LOAD_VERTICAL_LAYOUT )
      {
#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
        /* no fast retrieval for blended MM fonts without VVAR table */
        if ( ( FT_IS_NAMED_INSTANCE( face ) || FT_IS_VARIATION( face ) ) &&
             !( ttface->variation_support & TT_FACE_FLAG_VAR_VADVANCE )  )
          return FT_THROW( Unimplemented_Feature );
#endif

        /* check whether we have data from the `vmtx' table at all; */
        /* otherwise we extract the info from the CFF glyphstrings  */
        /* (instead of synthesizing a global value using the `OS/2' */
        /* table)                                                   */
        if ( !ttface->vertical_info )
          goto Missing_Table;

        for ( nn = 0; nn < count; nn++ )
        {
          FT_UShort  ah;


          ( (SFNT_Service)ttface->sfnt )->get_metrics( ttface,
                                                       1,
                                                       start + nn,
                                                       &dummy,
                                                       &ah );

          FT_TRACE5(( "  idx %d: advance height %d font unit%s\n",
                      start + nn,
                      ah,
                      ah == 1 ? "" : "s" ));
          advances[nn] = ah;
        }
      }
      else
      {
#ifdef TT_CONFIG_OPTION_GX_VAR_SUPPORT
        /* no fast retrieval for blended MM fonts without HVAR table */
        if ( ( FT_IS_NAMED_INSTANCE( face ) || FT_IS_VARIATION( face ) ) &&
             !( ttface->variation_support & TT_FACE_FLAG_VAR_HADVANCE )  )
          return FT_THROW( Unimplemented_Feature );
#endif

        /* check whether we have data from the `hmtx' table at all */
        if ( !ttface->horizontal.number_Of_HMetrics )
          goto Missing_Table;

        for ( nn = 0; nn < count; nn++ )
        {
          FT_UShort  aw;


          ( (SFNT_Service)ttface->sfnt )->get_metrics( ttface,
                                                       0,
                                                       start + nn,
                                                       &dummy,
                                                       &aw );

          FT_TRACE5(( "  idx %d: advance width %d font unit%s\n",
                      start + nn,
                      aw,
                      aw == 1 ? "" : "s" ));
          advances[nn] = aw;
        }
      }

      return error;
    }

  Missing_Table:
    flags |= (FT_UInt32)FT_LOAD_ADVANCE_ONLY;

    for ( nn = 0; nn < count; nn++ )
    {
      error = cff_glyph_load( slot, face->size, start + nn, flags );
      if ( error )
        break;

      advances[nn] = ( flags & FT_LOAD_VERTICAL_LAYOUT )
                     ? slot->linearVertAdvance
                     : slot->linearHoriAdvance;
    }

    return error;
  }


  /*
   *  GLYPH DICT SERVICE
   *
   */

  static FT_Error
  cff_get_glyph_name( CFF_Face    face,
                      FT_UInt     glyph_index,
                      FT_Pointer  buffer,
                      FT_UInt     buffer_max )
  {
    CFF_Font    font   = (CFF_Font)face->extra.data;
    FT_String*  gname;
    FT_UShort   sid;
    FT_Error    error;


    /* CFF2 table does not have glyph names; */
    /* we need to use `post' table method    */
    if ( font->version_major == 2 )
    {
      FT_Library            library     = FT_FACE_LIBRARY( face );
      FT_Module             sfnt_module = FT_Get_Module( library, "sfnt" );
      FT_Service_GlyphDict  service     =
        (FT_Service_GlyphDict)ft_module_get_service(
                                 sfnt_module,
                                 FT_SERVICE_ID_GLYPH_DICT,
                                 0 );


      if ( service && service->get_name )
        return service->get_name( FT_FACE( face ),
                                  glyph_index,
                                  buffer,
                                  buffer_max );
      else
      {
        FT_ERROR(( "cff_get_glyph_name:"
                   " cannot get glyph name from a CFF2 font\n"
                   "                   "
                   " without the `PSNames' module\n" ));
        error = FT_THROW( Missing_Module );
        goto Exit;
      }
    }

    if ( !font->psnames )
    {
      FT_ERROR(( "cff_get_glyph_name:"
                 " cannot get glyph name from CFF & CEF fonts\n"
                 "                   "
                 " without the `PSNames' module\n" ));
      error = FT_THROW( Missing_Module );
      goto Exit;
    }

    /* first, locate the sid in the charset table */
    sid = font->charset.sids[glyph_index];

    /* now, lookup the name itself */
    gname = cff_index_get_sid_string( font, sid );

    if ( gname )
      FT_STRCPYN( buffer, gname, buffer_max );

    error = FT_Err_Ok;

  Exit:
    return error;
  }


  FT_DEFINE_SERVICE_GLYPHDICTREC(
    cff_service_glyph_dict,

    (FT_GlyphDict_GetNameFunc)  cff_get_glyph_name       /* get_name   */
  )


  /*
   * TT CMAP INFO
   *
   * If the charmap is a synthetic Unicode encoding cmap or
   * a Type 1 standard (or expert) encoding cmap, hide TT CMAP INFO
   * service defined in SFNT module.
   *
   * Otherwise call the service function in the sfnt module.
   *
   */
  static FT_Error
  cff_get_cmap_info( FT_CharMap    charmap,
                     TT_CMapInfo  *cmap_info )
  {
    FT_CMap   cmap  = FT_CMAP( charmap );
    FT_Error  error = FT_Err_Ok;

    FT_Face     face    = FT_CMAP_FACE( cmap );
    FT_Library  library = FT_FACE_LIBRARY( face );


    if ( cmap->clazz != &cff_cmap_encoding_class_rec &&
         cmap->clazz != &cff_cmap_unicode_class_rec  )
    {
      FT_Module           sfnt    = FT_Get_Module( library, "sfnt" );
      FT_Service_TTCMaps  service =
        (FT_Service_TTCMaps)ft_module_get_service( sfnt,
                                                   FT_SERVICE_ID_TT_CMAP,
                                                   0 );


      if ( service && service->get_cmap_info )
        error = service->get_cmap_info( charmap, cmap_info );
    }
    else
      error = FT_THROW( Invalid_CharMap_Format );

    return error;
  }


  FT_DEFINE_SERVICE_TTCMAPSREC(
    cff_service_get_cmap_info,

    (TT_CMap_Info_GetFunc)cff_get_cmap_info    /* get_cmap_info */
  )


  /*
   *  CFFLOAD SERVICE
   *
   */

  FT_DEFINE_SERVICE_CFFLOADREC(
    cff_service_cff_load,

    (FT_Get_Standard_Encoding_Func)cff_get_standard_encoding,
    (FT_Load_Private_Dict_Func)    cff_load_private_dict,
    (FT_FD_Select_Get_Func)        cff_fd_select_get,
    (FT_Blend_Check_Vector_Func)   cff_blend_check_vector,
    (FT_Blend_Build_Vector_Func)   cff_blend_build_vector
  )


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /****                                                                 ****/
  /****                                                                 ****/
  /****                D R I V E R  I N T E R F A C E                   ****/
  /****                                                                 ****/
  /****                                                                 ****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/

#if !defined FT_CONFIG_OPTION_NO_GLYPH_NAMES
  FT_DEFINE_SERVICEDESCREC4(
    cff_services,

    FT_SERVICE_ID_FONT_FORMAT,          FT_FONT_FORMAT_CFF,
    FT_SERVICE_ID_GLYPH_DICT,           &cff_service_glyph_dict,
    FT_SERVICE_ID_TT_CMAP,              &cff_service_get_cmap_info,
    FT_SERVICE_ID_CFF_LOAD,             &cff_service_cff_load
  )
#else
  FT_DEFINE_SERVICEDESCREC3(
    cff_services,

    FT_SERVICE_ID_FONT_FORMAT,          FT_FONT_FORMAT_CFF,
    FT_SERVICE_ID_TT_CMAP,              &cff_service_get_cmap_info,
    FT_SERVICE_ID_CFF_LOAD,             &cff_service_cff_load
  )
#endif


  FT_CALLBACK_DEF( FT_Module_Interface )
  cff_get_interface( FT_Module    driver,       /* CFF_Driver */
                     const char*  module_interface )
  {
    FT_Library           library;
    FT_Module            sfnt;
    FT_Module_Interface  result;


    result = ft_service_list_lookup( cff_services, module_interface );
    if ( result )
      return result;

    if ( !driver )
      return NULL;
    library = driver->library;
    if ( !library )
      return NULL;

    /* we pass our request to the `sfnt' module */
    sfnt = FT_Get_Module( library, "sfnt" );

    return sfnt ? sfnt->clazz->get_interface( sfnt, module_interface ) : 0;
  }


  /* The FT_DriverInterface structure is defined in ftdriver.h. */

#ifdef TT_CONFIG_OPTION_EMBEDDED_BITMAPS
#define CFF_SIZE_SELECT cff_size_select
#else
#define CFF_SIZE_SELECT 0
#endif

  FT_DEFINE_DRIVER(
    cff_driver_class,

      FT_MODULE_FONT_DRIVER          |
      FT_MODULE_DRIVER_SCALABLE      |
      FT_MODULE_DRIVER_HAS_HINTER    |
      FT_MODULE_DRIVER_HINTS_LIGHTLY,

      sizeof ( PS_DriverRec ),
      "cff",
      0x10000L,
      0x20000L,

      NULL,   /* module-specific interface */

      cff_driver_init,          /* FT_Module_Constructor  module_init   */
      cff_driver_done,          /* FT_Module_Destructor   module_done   */
      cff_get_interface,        /* FT_Module_Requester    get_interface */

    sizeof ( TT_FaceRec ),
    sizeof ( CFF_SizeRec ),
    sizeof ( CFF_GlyphSlotRec ),

    cff_face_init,              /* FT_Face_InitFunc  init_face */
    cff_face_done,              /* FT_Face_DoneFunc  done_face */
    cff_size_init,              /* FT_Size_InitFunc  init_size */
    cff_size_done,              /* FT_Size_DoneFunc  done_size */
    cff_slot_init,              /* FT_Slot_InitFunc  init_slot */
    cff_slot_done,              /* FT_Slot_DoneFunc  done_slot */

    cff_glyph_load,             /* FT_Slot_LoadFunc  load_glyph */

    NULL,                       /* FT_Face_GetKerningFunc   get_kerning  */
    NULL,                       /* FT_Face_AttachFunc       attach_file  */
    cff_get_advances,           /* FT_Face_GetAdvancesFunc  get_advances */

    cff_size_request,           /* FT_Size_RequestFunc  request_size */
    CFF_SIZE_SELECT             /* FT_Size_SelectFunc   select_size  */
  )


/* END */
