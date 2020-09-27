/***************************************************************************/
/*                                                                         */
/*  tttables.h                                                             */
/*                                                                         */
/*    Basic SFNT/TrueType tables definitions and interface                 */
/*    (specification only).                                                */
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


#ifndef TTTABLES_H_
#define TTTABLES_H_


#include "ft2build.h"
#include FT_FREETYPE_H


FT_BEGIN_HEADER

  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    truetype_tables                                                    */
  /*                                                                       */
  /* <Title>                                                               */
  /*    TrueType Tables                                                    */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    TrueType specific table types and functions.                       */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This section contains definitions of some basic tables specific to */
  /*    TrueType and OpenType as well as some routines used to access and  */
  /*    process them.                                                      */
  /*                                                                       */
  /* <Order>                                                               */
  /*    TT_Header                                                          */
  /*    TT_HoriHeader                                                      */
  /*    TT_VertHeader                                                      */
  /*    TT_OS2                                                             */
  /*    TT_Postscript                                                      */
  /*    TT_PCLT                                                            */
  /*    TT_MaxProfile                                                      */
  /*                                                                       */
  /*    FT_Sfnt_Tag                                                        */
  /*    FT_Get_Sfnt_Table                                                  */
  /*    FT_Load_Sfnt_Table                                                 */
  /*    FT_Sfnt_Table_Info                                                 */
  /*                                                                       */
  /*    FT_Get_CMap_Format                                                 */
  /*                                                                       */
  /*    FT_PARAM_TAG_UNPATENTED_HINTING                                    */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    TT_Header                                                          */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure to model a TrueType font header table.  All fields     */
  /*    follow the OpenType specification.                                 */
  /*                                                                       */
  typedef struct  TT_Header_
  {
    FT_Fixed   Table_Version;
    FT_Fixed   Font_Revision;

    FT_Long    CheckSum_Adjust;
    FT_Long    Magic_Number;

    FT_UShort  Flags;
    FT_UShort  Units_Per_EM;

    FT_Long    Created [2];
    FT_Long    Modified[2];

    FT_Short   xMin;
    FT_Short   yMin;
    FT_Short   xMax;
    FT_Short   yMax;

    FT_UShort  Mac_Style;
    FT_UShort  Lowest_Rec_PPEM;

    FT_Short   Font_Direction;
    FT_Short   Index_To_Loc_Format;
    FT_Short   Glyph_Data_Format;

  } TT_Header;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    TT_HoriHeader                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure to model a TrueType horizontal header, the `hhea'      */
  /*    table, as well as the corresponding horizontal metrics table,      */
  /*    `hmtx'.                                                            */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    Version                :: The table version.                       */
  /*                                                                       */
  /*    Ascender               :: The font's ascender, i.e., the distance  */
  /*                              from the baseline to the top-most of all */
  /*                              glyph points found in the font.          */
  /*                                                                       */
  /*                              This value is invalid in many fonts, as  */
  /*                              it is usually set by the font designer,  */
  /*                              and often reflects only a portion of the */
  /*                              glyphs found in the font (maybe ASCII).  */
  /*                                                                       */
  /*                              You should use the `sTypoAscender' field */
  /*                              of the `OS/2' table instead if you want  */
  /*                              the correct one.                         */
  /*                                                                       */
  /*    Descender              :: The font's descender, i.e., the distance */
  /*                              from the baseline to the bottom-most of  */
  /*                              all glyph points found in the font.  It  */
  /*                              is negative.                             */
  /*                                                                       */
  /*                              This value is invalid in many fonts, as  */
  /*                              it is usually set by the font designer,  */
  /*                              and often reflects only a portion of the */
  /*                              glyphs found in the font (maybe ASCII).  */
  /*                                                                       */
  /*                              You should use the `sTypoDescender'      */
  /*                              field of the `OS/2' table instead if you */
  /*                              want the correct one.                    */
  /*                                                                       */
  /*    Line_Gap               :: The font's line gap, i.e., the distance  */
  /*                              to add to the ascender and descender to  */
  /*                              get the BTB, i.e., the                   */
  /*                              baseline-to-baseline distance for the    */
  /*                              font.                                    */
  /*                                                                       */
  /*    advance_Width_Max      :: This field is the maximum of all advance */
  /*                              widths found in the font.  It can be     */
  /*                              used to compute the maximum width of an  */
  /*                              arbitrary string of text.                */
  /*                                                                       */
  /*    min_Left_Side_Bearing  :: The minimum left side bearing of all     */
  /*                              glyphs within the font.                  */
  /*                                                                       */
  /*    min_Right_Side_Bearing :: The minimum right side bearing of all    */
  /*                              glyphs within the font.                  */
  /*                                                                       */
  /*    xMax_Extent            :: The maximum horizontal extent (i.e., the */
  /*                              `width' of a glyph's bounding box) for   */
  /*                              all glyphs in the font.                  */
  /*                                                                       */
  /*    caret_Slope_Rise       :: The rise coefficient of the cursor's     */
  /*                              slope of the cursor (slope=rise/run).    */
  /*                                                                       */
  /*    caret_Slope_Run        :: The run coefficient of the cursor's      */
  /*                              slope.                                   */
  /*                                                                       */
  /*    caret_Offset           :: The cursor's offset for slanted fonts.   */
  /*                                                                       */
  /*    Reserved               :: 8~reserved bytes.                        */
  /*                                                                       */
  /*    metric_Data_Format     :: Always~0.                                */
  /*                                                                       */
  /*    number_Of_HMetrics     :: Number of HMetrics entries in the `hmtx' */
  /*                              table -- this value can be smaller than  */
  /*                              the total number of glyphs in the font.  */
  /*                                                                       */
  /*    long_metrics           :: A pointer into the `hmtx' table.         */
  /*                                                                       */
  /*    short_metrics          :: A pointer into the `hmtx' table.         */
  /*                                                                       */
  /* <Note>                                                                */
  /*    For an OpenType variation font, the values of the following fields */
  /*    can change after a call to @FT_Set_Var_Design_Coordinates (and     */
  /*    friends) if the font contains an `MVAR' table: `caret_Slope_Rise', */
  /*    `caret_Slope_Run', and `caret_Offset'.                             */
  /*                                                                       */
  typedef struct  TT_HoriHeader_
  {
    FT_Fixed   Version;
    FT_Short   Ascender;
    FT_Short   Descender;
    FT_Short   Line_Gap;

    FT_UShort  advance_Width_Max;      /* advance width maximum */

    FT_Short   min_Left_Side_Bearing;  /* minimum left-sb       */
    FT_Short   min_Right_Side_Bearing; /* minimum right-sb      */
    FT_Short   xMax_Extent;            /* xmax extents          */
    FT_Short   caret_Slope_Rise;
    FT_Short   caret_Slope_Run;
    FT_Short   caret_Offset;

    FT_Short   Reserved[4];

    FT_Short   metric_Data_Format;
    FT_UShort  number_Of_HMetrics;

    /* The following fields are not defined by the OpenType specification */
    /* but they are used to connect the metrics header to the relevant    */
    /* `hmtx' table.                                                      */

    void*      long_metrics;
    void*      short_metrics;

  } TT_HoriHeader;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    TT_VertHeader                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure used to model a TrueType vertical header, the `vhea'   */
  /*    table, as well as the corresponding vertical metrics table,        */
  /*    `vmtx'.                                                            */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    Version                 :: The table version.                      */
  /*                                                                       */
  /*    Ascender                :: The font's ascender, i.e., the distance */
  /*                               from the baseline to the top-most of    */
  /*                               all glyph points found in the font.     */
  /*                                                                       */
  /*                               This value is invalid in many fonts, as */
  /*                               it is usually set by the font designer, */
  /*                               and often reflects only a portion of    */
  /*                               the glyphs found in the font (maybe     */
  /*                               ASCII).                                 */
  /*                                                                       */
  /*                               You should use the `sTypoAscender'      */
  /*                               field of the `OS/2' table instead if    */
  /*                               you want the correct one.               */
  /*                                                                       */
  /*    Descender               :: The font's descender, i.e., the         */
  /*                               distance from the baseline to the       */
  /*                               bottom-most of all glyph points found   */
  /*                               in the font.  It is negative.           */
  /*                                                                       */
  /*                               This value is invalid in many fonts, as */
  /*                               it is usually set by the font designer, */
  /*                               and often reflects only a portion of    */
  /*                               the glyphs found in the font (maybe     */
  /*                               ASCII).                                 */
  /*                                                                       */
  /*                               You should use the `sTypoDescender'     */
  /*                               field of the `OS/2' table instead if    */
  /*                               you want the correct one.               */
  /*                                                                       */
  /*    Line_Gap                :: The font's line gap, i.e., the distance */
  /*                               to add to the ascender and descender to */
  /*                               get the BTB, i.e., the                  */
  /*                               baseline-to-baseline distance for the   */
  /*                               font.                                   */
  /*                                                                       */
  /*    advance_Height_Max      :: This field is the maximum of all        */
  /*                               advance heights found in the font.  It  */
  /*                               can be used to compute the maximum      */
  /*                               height of an arbitrary string of text.  */
  /*                                                                       */
  /*    min_Top_Side_Bearing    :: The minimum top side bearing of all     */
  /*                               glyphs within the font.                 */
  /*                                                                       */
  /*    min_Bottom_Side_Bearing :: The minimum bottom side bearing of all  */
  /*                               glyphs within the font.                 */
  /*                                                                       */
  /*    yMax_Extent             :: The maximum vertical extent (i.e., the  */
  /*                               `height' of a glyph's bounding box) for */
  /*                               all glyphs in the font.                 */
  /*                                                                       */
  /*    caret_Slope_Rise        :: The rise coefficient of the cursor's    */
  /*                               slope of the cursor (slope=rise/run).   */
  /*                                                                       */
  /*    caret_Slope_Run         :: The run coefficient of the cursor's     */
  /*                               slope.                                  */
  /*                                                                       */
  /*    caret_Offset            :: The cursor's offset for slanted fonts.  */
  /*                                                                       */
  /*    Reserved                :: 8~reserved bytes.                       */
  /*                                                                       */
  /*    metric_Data_Format      :: Always~0.                               */
  /*                                                                       */
  /*    number_Of_VMetrics      :: Number of VMetrics entries in the       */
  /*                               `vmtx' table -- this value can be       */
  /*                               smaller than the total number of glyphs */
  /*                               in the font.                            */
  /*                                                                       */
  /*    long_metrics            :: A pointer into the `vmtx' table.        */
  /*                                                                       */
  /*    short_metrics           :: A pointer into the `vmtx' table.        */
  /*                                                                       */
  /* <Note>                                                                */
  /*    For an OpenType variation font, the values of the following fields */
  /*    can change after a call to @FT_Set_Var_Design_Coordinates (and     */
  /*    friends) if the font contains an `MVAR' table: `Ascender',         */
  /*    `Descender', `Line_Gap', `caret_Slope_Rise', `caret_Slope_Run',    */
  /*    and `caret_Offset'.                                                */
  /*                                                                       */
  typedef struct  TT_VertHeader_
  {
    FT_Fixed   Version;
    FT_Short   Ascender;
    FT_Short   Descender;
    FT_Short   Line_Gap;

    FT_UShort  advance_Height_Max;      /* advance height maximum */

    FT_Short   min_Top_Side_Bearing;    /* minimum top-sb          */
    FT_Short   min_Bottom_Side_Bearing; /* minimum bottom-sb       */
    FT_Short   yMax_Extent;             /* ymax extents            */
    FT_Short   caret_Slope_Rise;
    FT_Short   caret_Slope_Run;
    FT_Short   caret_Offset;

    FT_Short   Reserved[4];

    FT_Short   metric_Data_Format;
    FT_UShort  number_Of_VMetrics;

    /* The following fields are not defined by the OpenType specification */
    /* but they are used to connect the metrics header to the relevant    */
    /* `vmtx' table.                                                      */

    void*      long_metrics;
    void*      short_metrics;

  } TT_VertHeader;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    TT_OS2                                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure to model a TrueType `OS/2' table.  All fields comply   */
  /*    to the OpenType specification.                                     */
  /*                                                                       */
  /*    Note that we now support old Mac fonts that do not include an      */
  /*    `OS/2' table.  In this case, the `version' field is always set to  */
  /*    0xFFFF.                                                            */
  /*                                                                       */
  /* <Note>                                                                */
  /*    For an OpenType variation font, the values of the following fields */
  /*    can change after a call to @FT_Set_Var_Design_Coordinates (and     */
  /*    friends) if the font contains an `MVAR' table: `sCapHeight',       */
  /*    `sTypoAscender', `sTypoDescender', `sTypoLineGap', `sxHeight',     */
  /*    `usWinAscent', `usWinDescent', `yStrikeoutPosition',               */
  /*    `yStrikeoutSize', `ySubscriptXOffset', `ySubScriptXSize',          */
  /*    `ySubscriptYOffset', `ySubscriptYSize', `ySuperscriptXOffset',     */
  /*    `ySuperscriptXSize', `ySuperscriptYOffset', and                    */
  /*    `ySuperscriptYSize'.                                               */
  /*                                                                       */
  /*    Possible values for bits in the `ulUnicodeRangeX' fields are given */
  /*    by the @TT_UCR_XXX macros.                                         */
  /*                                                                       */

  typedef struct  TT_OS2_
  {
    FT_UShort  version;                /* 0x0001 - more or 0xFFFF */
    FT_Short   xAvgCharWidth;
    FT_UShort  usWeightClass;
    FT_UShort  usWidthClass;
    FT_UShort  fsType;
    FT_Short   ySubscriptXSize;
    FT_Short   ySubscriptYSize;
    FT_Short   ySubscriptXOffset;
    FT_Short   ySubscriptYOffset;
    FT_Short   ySuperscriptXSize;
    FT_Short   ySuperscriptYSize;
    FT_Short   ySuperscriptXOffset;
    FT_Short   ySuperscriptYOffset;
    FT_Short   yStrikeoutSize;
    FT_Short   yStrikeoutPosition;
    FT_Short   sFamilyClass;

    FT_Byte    panose[10];

    FT_ULong   ulUnicodeRange1;        /* Bits 0-31   */
    FT_ULong   ulUnicodeRange2;        /* Bits 32-63  */
    FT_ULong   ulUnicodeRange3;        /* Bits 64-95  */
    FT_ULong   ulUnicodeRange4;        /* Bits 96-127 */

    FT_Char    achVendID[4];

    FT_UShort  fsSelection;
    FT_UShort  usFirstCharIndex;
    FT_UShort  usLastCharIndex;
    FT_Short   sTypoAscender;
    FT_Short   sTypoDescender;
    FT_Short   sTypoLineGap;
    FT_UShort  usWinAscent;
    FT_UShort  usWinDescent;

    /* only version 1 and higher: */

    FT_ULong   ulCodePageRange1;       /* Bits 0-31   */
    FT_ULong   ulCodePageRange2;       /* Bits 32-63  */

    /* only version 2 and higher: */

    FT_Short   sxHeight;
    FT_Short   sCapHeight;
    FT_UShort  usDefaultChar;
    FT_UShort  usBreakChar;
    FT_UShort  usMaxContext;

    /* only version 5 and higher: */

    FT_UShort  usLowerOpticalPointSize;       /* in twips (1/20th points) */
    FT_UShort  usUpperOpticalPointSize;       /* in twips (1/20th points) */

  } TT_OS2;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    TT_Postscript                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A structure to model a TrueType `post' table.  All fields comply   */
  /*    to the OpenType specification.  This structure does not reference  */
  /*    a font's PostScript glyph names; use @FT_Get_Glyph_Name to         */
  /*    retrieve them.                                                     */
  /*                                                                       */
  /* <Note>                                                                */
  /*    For an OpenType variation font, the values of the following fields */
  /*    can change after a call to @FT_Set_Var_Design_Coordinates (and     */
  /*    friends) if the font contains an `MVAR' table: `underlinePosition' */
  /*    and `underlineThickness'.                                          */
  /*                                                                       */
  typedef struct  TT_Postscript_
  {
    FT_Fixed  FormatType;
    FT_Fixed  italicAngle;
    FT_Short  underlinePosition;
    FT_Short  underlineThickness;
    FT_ULong  isFixedPitch;
    FT_ULong  minMemType42;
    FT_ULong  maxMemType42;
    FT_ULong  minMemType1;
    FT_ULong  maxMemType1;

    /* Glyph names follow in the `post' table, but we don't */
    /* load them by default.                                */

  } TT_Postscript;


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    TT_MaxProfile                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The maximum profile (`maxp') table contains many max values, which */
  /*    can be used to pre-allocate arrays for speeding up glyph loading   */
  /*    and hinting.                                                       */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    version               :: The version number.                       */
  /*                                                                       */
  /*    numGlyphs             :: The number of glyphs in this TrueType     */
  /*                             font.                                     */
  /*                                                                       */
  /*    maxPoints             :: The maximum number of points in a         */
  /*                             non-composite TrueType glyph.  See also   */
  /*                             `maxCompositePoints'.                     */
  /*                                                                       */
  /*    maxContours           :: The maximum number of contours in a       */
  /*                             non-composite TrueType glyph.  See also   */
  /*                             `maxCompositeContours'.                   */
  /*                                                                       */
  /*    maxCompositePoints    :: The maximum number of points in a         */
  /*                             composite TrueType glyph.  See also       */
  /*                             `maxPoints'.                              */
  /*                                                                       */
  /*    maxCompositeContours  :: The maximum number of contours in a       */
  /*                             composite TrueType glyph.  See also       */
  /*                             `maxContours'.                            */
  /*                                                                       */
  /*    maxZones              :: The maximum number of zones used for      */
  /*                             glyph hinting.                            */
  /*                                                                       */
  /*    maxTwilightPoints     :: The maximum number of points in the       */
  /*                             twilight zone used for glyph hinting.     */
  /*                                                                       */
  /*    maxStorage            :: The maximum number of elements in the     */
  /*                             storage area used for glyph hinting.      */
  /*                                                                       */
  /*    maxFunctionDefs       :: The maximum number of function            */
  /*                             definitions in the TrueType bytecode for  */
  /*                             this font.                                */
  /*                                                                       */
  /*    maxInstructionDefs    :: The maximum number of instruction         */
  /*                             definitions in the TrueType bytecode for  */
  /*                             this font.                                */
  /*                                                                       */
  /*    maxStackElements      :: The maximum number of stack elements used */
  /*                             during bytecode interpretation.           */
  /*                                                                       */
  /*    maxSizeOfInstructions :: The maximum number of TrueType opcodes    */
  /*                             used for glyph hinting.                   */
  /*                                                                       */
  /*    maxComponentElements  :: The maximum number of simple (i.e., non-  */
  /*                             composite) glyphs in a composite glyph.   */
  /*                                                                       */
  /*    maxComponentDepth     :: The maximum nesting depth of composite    */
  /*                             glyphs.                                   */
  /*                                                                       */
  /* <Note>                                                                */
  /*    This structure is only used during font loading.                   */
  /*                                                                       */
  typedef struct  TT_MaxProfile_
  {
    FT_Fixed   version;
    FT_UShort  numGlyphs;
    FT_UShort  maxPoints;
    FT_UShort  maxContours;
    FT_UShort  maxCompositePoints;
    FT_UShort  maxCompositeContours;
    FT_UShort  maxZones;
    FT_UShort  maxTwilightPoints;
    FT_UShort  maxStorage;
    FT_UShort  maxFunctionDefs;
    FT_UShort  maxInstructionDefs;
    FT_UShort  maxStackElements;
    FT_UShort  maxSizeOfInstructions;
    FT_UShort  maxComponentElements;
    FT_UShort  maxComponentDepth;

  } TT_MaxProfile;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_CMap_Format                                                 */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Return the format of an SFNT `cmap' table.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    charmap ::                                                         */
  /*      The target charmap.                                              */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The format of `charmap'.  If `charmap' doesn't belong to an SFNT   */
  /*    face, return -1.                                                   */
  /*                                                                       */
  FT_EXPORT( FT_Long )
  FT_Get_CMap_Format( FT_CharMap  charmap );

  /* */


FT_END_HEADER

#endif /* TTTABLES_H_ */


/* END */
