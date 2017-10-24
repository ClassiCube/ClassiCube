#ifndef CC_FRUSTUMCULLING_H
#define CC_FRUSTUMCULLING_H
#include "Matrix.h"
#include "Typedefs.h"
/* Implements frustum culling of bounding spheres.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
   Sourced from http://www.crownandcutlass.com/features/technicaldetails/frustum.html
*/

/* Returns whether the given bounding sphere is in the view frustum. */
bool FrustumCulling_SphereInFrustum(Real32 x, Real32 y, Real32 z, Real32 radius);
/* Calculates the planes that constitute view frustum. */
void FrustumCulling_CalcFrustumEquations(Matrix* projection, Matrix* modelView);
#endif