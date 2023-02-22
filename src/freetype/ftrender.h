/***************************************************************************/
/*                                                                         */
/*  ftrender.h                                                             */
/*                                                                         */
/*    FreeType renderer modules public interface (specification).          */
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


#ifndef FTRENDER_H_
#define FTRENDER_H_


#include "ft2build.h"
#include FT_MODULE_H
#include FT_GLYPH_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    module_management                                                  */
  /*                                                                       */
  /*************************************************************************/


  /* create a new glyph object */
  typedef FT_Error
  (*FT_Glyph_InitFunc)( FT_Glyph      glyph,
                        FT_GlyphSlot  slot );

  /* destroys a given glyph object */
  typedef void
  (*FT_Glyph_DoneFunc)( FT_Glyph  glyph );

  typedef void
  (*FT_Glyph_TransformFunc)( FT_Glyph          glyph,
                             const FT_Matrix*  matrix,
                             const FT_Vector*  delta );

  typedef void
  (*FT_Glyph_GetBBoxFunc)( FT_Glyph  glyph,
                           FT_BBox*  abbox );

  typedef FT_Error
  (*FT_Glyph_PrepareFunc)( FT_Glyph      glyph,
                           FT_GlyphSlot  slot );


  struct  FT_Glyph_Class_
  {
    FT_Long                 glyph_size;
    FT_Glyph_Format         glyph_format;

    FT_Glyph_InitFunc       glyph_init;
    FT_Glyph_DoneFunc       glyph_done;
    FT_Glyph_TransformFunc  glyph_transform;
    FT_Glyph_GetBBoxFunc    glyph_bbox;
    FT_Glyph_PrepareFunc    glyph_prepare;
  };


  typedef FT_Error
  (*FT_Renderer_RenderFunc)( FT_Renderer       renderer,
                             FT_GlyphSlot      slot,
                             FT_Render_Mode    mode,
                             const FT_Vector*  origin );


  typedef void
  (*FT_Renderer_GetCBoxFunc)( FT_Renderer   renderer,
                              FT_GlyphSlot  slot,
                              FT_BBox*      cbox );


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Renderer_Class                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The renderer module class descriptor.                              */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    root            :: The root @FT_Module_Class fields.               */
  /*                                                                       */
  /*    render_glyph    :: A method used to render the image that is in a  */
  /*                       given glyph slot into a bitmap.                 */
  /*                                                                       */
  /*    raster_class    :: For @FT_GLYPH_FORMAT_OUTLINE renderers only.    */
  /*                       This is a pointer to its raster's class.        */
  /*                                                                       */
  typedef struct  FT_Renderer_Class_
  {
    FT_Module_Class            root;

    FT_Renderer_RenderFunc     render_glyph;

    FT_Raster_Funcs*           raster_class;

  } FT_Renderer_Class;

  /* */


FT_END_HEADER

#endif /* FTRENDER_H_ */


/* END */
