/* KallistiGL for KallistiOS ##version##

   libgl/gl-error.c
   Copyright (C) 2014 Josh Pearson
   Copyright (C) 2016 Lawrence Sebald
 * Copyright (C) 2017 Luke Benstead

    KOS Open GL State Machine Error Code Implementation.
*/

#include <stdio.h>

#include "private.h"

GLenum LAST_ERROR = GL_NO_ERROR;
char ERROR_FUNCTION[64] = { '\0' };

/* Quoth the GL Spec:
    When an error occurs, the error flag is set to the appropriate error code
    value. No other errors are recorded until glGetError is called, the error
    code is returned, and the flag is reset to GL_NO_ERROR.

   So, we only record an error here if the error code is currently unset.
   Nothing in the spec requires recording multiple error flags, although it is
   allowed by the spec. We take the easy way out for now. */


GLenum glGetError(void) {
    GLenum rv = LAST_ERROR;
    _glKosResetError();
    return rv;
}
