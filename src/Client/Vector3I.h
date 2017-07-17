#ifndef CS_VECTOR3I_H
#define CS_VECTOR3I_H
#include "Typedefs.h"
#include "Vectors.h"
#include "Compiler.h"
/* Represents 3 dimensional integer vector.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct Vector3I_ { Int32 X, Y, Z; } Vector3I;

/* Constructs a 3D vector with X, Y and Z set to given value. */
HINT_INLINE Vector3I Vector3I_Create1(Int32 value);

/* Constructs a 3D vector from the given coordinates. */
HINT_INLINE Vector3I Vector3I_Create3(Int32 x, Int32 y, Int32 z);


#define Vector3I_UnitX Vector3I_Create3(1, 0, 0)
#define Vector3I_UnitY Vector3I_Create3(0, 1, 0)
#define Vector3I_UnitZ Vector3I_Create3(0, 0, 1)
#define Vector3I_Zero Vector3I_Create3(0, 0, 0)
#define Vector3I_One Vector3I_Create3(1, 1, 1)
#define Vector3I_MinusOne Vector3I_Create3(-1, -1, -1)


/* Adds a and b. */
HINT_INLINE void Vector3I_Add(Vector3I* result, Vector3I* a, Vector3I* b);

/* Subtracts b from a. */
HINT_INLINE void Vector3I_Subtract(Vector3I* result, Vector3I* a, Vector3I* b);

/* Multiplies all components of a by scale. */
HINT_INLINE void Vector3I_Multiply1(Vector3I* result, Vector3I* a, Int32 scale);

/* Negates components of a. */
HINT_INLINE void Vector3I_Negate(Vector3I* result, Vector3I* a);


/* Returns whether the two vectors are exact same on all axes. */
HINT_INLINE bool Vector3I_Equals(Vector3I* a, Vector3I* b);

/* Returns whether the two vectors are different on any axes. */
HINT_INLINE bool Vector3I_NotEquals(Vector3I* a, Vector3I* b);

/* Returns a vector such that each component is floor of input floating-point component.*/
HINT_INLINE void Vector3I_Floor(Vector3I* result, Vector3* a);

/* Returns a vector with the integer components converted to floating-point components.*/
HINT_INLINE void Vector3I_ToVector3(Vector3* result, Vector3I* a);

/* Returns a vector such that each component is minimum of corresponding a and b component.*/
HINT_INLINE void Vector3I_Min(Vector3I* result, Vector3I* a, Vector3I* b);

/* Returns a vector such that each component is maximum of corresponding a and b component.*/
HINT_INLINE void Vector3I_Max(Vector3I* result, Vector3I* a, Vector3I* b);
#endif