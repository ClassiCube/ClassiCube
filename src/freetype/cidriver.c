/***************************************************************************/
/*                                                                         */
/*  cidriver.c                                                             */
/*                                                                         */
/*    CID driver interface (body).                                         */
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
#include "cidriver.h"
#include "cidgload.h"
#include FT_INTERNAL_DEBUG_H

#include "ciderrs.h"

#include FT_SERVICE_FONT_FORMAT_H
#include FT_DRIVER_H

#include FT_INTERNAL_POSTSCRIPT_AUX_H


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_ciddriver


  /*
   *  SERVICE LIST
   *
   */

  static const FT_ServiceDescRec  cid_services[] =
  {
    { FT_SERVICE_ID_FONT_FORMAT,          FT_FONT_FORMAT_CID },
    { NULL, NULL }
  };


  FT_CALLBACK_DEF( FT_Module_Interface )
  cid_get_interface( FT_Module    module,
                     const char*  cid_interface )
  {
    FT_UNUSED( module );

    return ft_service_list_lookup( cid_services, cid_interface );
  }



  FT_CALLBACK_TABLE_DEF
  const FT_Driver_ClassRec  t1cid_driver_class =
  {
    {
      FT_MODULE_FONT_DRIVER       |
      FT_MODULE_DRIVER_SCALABLE   |
      FT_MODULE_DRIVER_HAS_HINTER,
      sizeof ( PS_DriverRec ),

      "t1cid",   /* module name           */
      0x10000L,  /* version 1.0 of driver */
      0x20000L,  /* requires FreeType 2.0 */

      NULL,    /* module-specific interface */

      cid_driver_init,          /* FT_Module_Constructor  module_init   */
      cid_driver_done,          /* FT_Module_Destructor   module_done   */
      cid_get_interface         /* FT_Module_Requester    get_interface */
    },

    sizeof ( CID_FaceRec ),
    sizeof ( CID_SizeRec ),
    sizeof ( CID_GlyphSlotRec ),

    cid_face_init,              /* FT_Face_InitFunc  init_face */
    cid_face_done,              /* FT_Face_DoneFunc  done_face */
    cid_size_init,              /* FT_Size_InitFunc  init_size */
    cid_size_done,              /* FT_Size_DoneFunc  done_size */
    cid_slot_init,              /* FT_Slot_InitFunc  init_slot */
    cid_slot_done,              /* FT_Slot_DoneFunc  done_slot */

    cid_slot_load_glyph,        /* FT_Slot_LoadFunc  load_glyph */

    NULL,                       /* FT_Face_GetKerningFunc   get_kerning  */
    NULL,                       /* FT_Face_AttachFunc       attach_file  */
    NULL,                       /* FT_Face_GetAdvancesFunc  get_advances */

    cid_size_request,           /* FT_Size_RequestFunc  request_size */
    NULL                        /* FT_Size_SelectFunc   select_size  */
  };


/* END */
