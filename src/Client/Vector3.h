#ifndef CS_VECTOR3_H
#define CS_VECTOR3_H
#include "Typedefs.h"
#include "Matrix.h"
/* Represents a 3 dimensional vector.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/


typedef struct Vector3 {
	Real32 X, Y, Z;
} Vector3;


/* Constructs a vector from the given coordinates. */
Vector3 Vector3_Create3(Real32 x, Real32 y, Real32 z);

/* Constructs a vector with X, Y and Z set to given value. */
Vector3 Vector3_Create1(Real32 value);

/* Returns the length of the given vector. */
Real32 Vector3_Length(Vector3* v);

/* Returns the squared length of the given vector. */
Real32 Vector3_LengthSquared(Vector3* v);


#define Vector3_UnitX Vector3_Create3(1.0f, 0.0f, 0.0f)

#define Vector3_UnitY Vector3_Create3(0.0f, 1.0f, 0.0f)

#define Vector3_UnitZ Vector3_Create3(0.0f, 0.0f, 1.0f)

#define Vector3_Zero Vector3_Create3(0.0f, 0.0f, 0.0f)

#define Vector3_One Vector3_Create3(1.0f, 1.0f, 1.0f)


/* Adds a and b. */
void Vector3_Add(Vector3* a, Vector3* b, Vector3* result);

/* Subtracts b from a. */
void Vector3_Subtract(Vector3* a, Vector3* b, Vector3* result);

/* Multiplies all components of a by scale. */
void Vector3_Multiply1(Vector3* a, Real32 scale, Vector3* result);

/* Multiplies components of a by scale. */
void Vector3_Multiply3(Vector3* a, Vector3* scale, Vector3* result);

/* Divides all components of a by scale. */
void Vector3_Divide1(Vector3* a, Real32 scale, Vector3* result);

/* Divides components of a by scale. */
void Vector3_Divide3(Vector3* a, Vector3* scale, Vector3* result);


/* Linearly interpolates between two vectors. */
void Vector3_Lerp(Vector3* a, Vector3* b, float blend, Vector3* result);

/* Calculates the dot product of two vectors. */
Real32 Vector3_Dot(Vector3* left, Vector3* right);

/* Calculates the cross product of two vectors. */
void Vector3_Cross(Vector3* a, Vector3* b, Vector3* result);

/* Calculates the normal of a vector. */
void Vector3_Normalize(Vector3* a, Vector3* result);

/* Transforms a vector by the given matrix. */
void Transform(Vector3* a, Matrix* mat, Vector3* result);

/* Transforms the Y component of a vector by the given matrix. */
void TransformY(Real32 y, Matrix* mat, Vector3* result);


/* Returns whether the two vectors are exact same on all axes. */
bool Vector3_Equals(Vector3* a, Vector3* b);

/* Returns whether the two vectors are different on any axis. */
bool Vector3_NotEquals(Vector3* a, Vector3* b);
#endif