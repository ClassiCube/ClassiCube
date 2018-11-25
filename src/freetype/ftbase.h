/***************************************************************************/
/*                                                                         */
/*  ftbase.h                                                               */
/*                                                                         */
/*    Private functions used in the `base' module (specification).         */
/*                                                                         */
/*  Copyright 2008-2018 by                                                 */
/*  David Turner, Robert Wilhelm, Werner Lemberg, and suzuki toshiya.      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef FTBASE_H_
#define FTBASE_H_


#include "ft2build.h"
#include FT_INTERNAL_OBJECTS_H


FT_BEGIN_HEADER

/* MacOS resource fork cannot exceed 16MB at least for Carbon code; */
/* see https://support.microsoft.com/en-us/kb/130437                */
#define FT_MAC_RFORK_MAX_LEN  0x00FFFFFFUL

FT_END_HEADER

#endif /* FTBASE_H_ */


/* END */
