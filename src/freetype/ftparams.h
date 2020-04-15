/***************************************************************************/
/*                                                                         */
/*  ftparams.h                                                             */
/*                                                                         */
/*    FreeType API for possible FT_Parameter tags (specification only).    */
/*                                                                         */
/*  Copyright 2017-2018 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef FTPARAMS_H_
#define FTPARAMS_H_

#include "ft2build.h"
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  /**************************************************************************
   *
   * @section:
   *   parameter_tags
   *
   * @title:
   *   Parameter Tags
   *
   * @abstract:
   *   Macros for driver property and font loading parameter tags.
   *
   * @description:
   *   This section contains macros for the @FT_Parameter structure that are
   *   used with various functions to activate some special functionality or
   *   different behaviour of various components of FreeType.
   *
   */


  /***************************************************************************
   *
   * @constant:
   *   FT_PARAM_TAG_IGNORE_TYPOGRAPHIC_FAMILY
   *
   * @description:
   *   A tag for @FT_Parameter to make @FT_Open_Face ignore typographic
   *   family names in the `name' table (introduced in OpenType version
   *   1.4).  Use this for backward compatibility with legacy systems that
   *   have a four-faces-per-family restriction.
   *
   * @since:
   *   2.8
   *
   */
#define FT_PARAM_TAG_IGNORE_TYPOGRAPHIC_FAMILY \
          FT_MAKE_TAG( 'i', 'g', 'p', 'f' )


  /***************************************************************************
   *
   * @constant:
   *   FT_PARAM_TAG_IGNORE_TYPOGRAPHIC_SUBFAMILY
   *
   * @description:
   *   A tag for @FT_Parameter to make @FT_Open_Face ignore typographic
   *   subfamily names in the `name' table (introduced in OpenType version
   *   1.4).  Use this for backward compatibility with legacy systems that
   *   have a four-faces-per-family restriction.
   *
   * @since:
   *   2.8
   *
   */
#define FT_PARAM_TAG_IGNORE_TYPOGRAPHIC_SUBFAMILY \
          FT_MAKE_TAG( 'i', 'g', 'p', 's' )


  /**************************************************************************
   *
   * @constant:
   *   FT_PARAM_TAG_STEM_DARKENING
   *
   * @description:
   *   An @FT_Parameter tag to be used with @FT_Face_Properties.  The
   *   corresponding Boolean argument specifies whether to apply stem
   *   darkening, overriding the global default values or the values set up
   *   with @FT_Property_Set (see @no-stem-darkening).
   *
   *   This is a passive setting that only takes effect if the font driver
   *   or autohinter honors it, which the CFF, Type~1, and CID drivers
   *   always do, but the autohinter only in `light' hinting mode (as of
   *   version 2.9).
   *
   * @since:
   *   2.8
   *
   */
#define FT_PARAM_TAG_STEM_DARKENING \
          FT_MAKE_TAG( 'd', 'a', 'r', 'k' )


  /* */


FT_END_HEADER


#endif /* FTPARAMS_H_ */


/* END */
