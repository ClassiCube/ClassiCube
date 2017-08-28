#ifndef CS_VECTORS_H
#define CS_VECTORS_H
#include "Typedefs.h"
#include "Matrix.h"
#include "Compiler.h"
/* Represents 2, 3 dimensional vectors.
   Copyright 2017 ClassicalSharp | Licensed under BSD-3
*/

typedef struct Vector2_ { Real32 X, Y; } Vector2;
typedef struct Vector3_ { Real32 X, Y, Z; } Vector3;
typedef struct Vector3I_ { Int32 X, Y, Z; } Vector3I;

/* Constructs a 2D vector from the given coordinates. */
Vector2 Vector2_Create2(Real32 x, Real32 y);
/* Constructs a 3D vector with X, Y and Z set to given value. */
Vector3 Vector3_Create1(Real32 value);
/* Constructs a 3D vector from the given coordinates. */
Vector3 Vector3_Create3(Real32 x, Real32 y, Real32 z);
/* Constructs a 3D vector with X, Y and Z set to given value. */
Vector3I Vector3I_Create1(Int32 value);
/* Constructs a 3D vector from the given coordinates. */
Vector3I Vector3I_Create3(Int32 x, Int32 y, Int32 z);

/* Returns the length of the given vector. */
Real32 Vector3_Length(Vector3* v);
/* Returns the squared length of the given vector. */
Real32 Vector3_LengthSquared(Vector3* v);

#define Vector2_UnitX Vector2_Create2(1.0f, 0.0f)
#define Vector2_UnitY Vector2_Create2(0.0f, 1.0f)
#define Vector2_Zero Vector2_Create2(0.0f, 0.0f)
#define Vector2_One Vector2_Create2(1.0f, 1.0f)

#define Vector3_UnitX Vector3_Create3(1.0f, 0.0f, 0.0f)
#define Vector3_UnitY Vector3_Create3(0.0f, 1.0f, 0.0f)
#define Vector3_UnitZ Vector3_Create3(0.0f, 0.0f, 1.0f)
#define Vector3_Zero Vector3_Create3(0.0f, 0.0f, 0.0f)
#define Vector3_One Vector3_Create3(1.0f, 1.0f, 1.0f)

#define Vector3I_UnitX Vector3I_Create3(1, 0, 0)
#define Vector3I_UnitY Vector3I_Create3(0, 1, 0)
#define Vector3I_UnitZ Vector3I_Create3(0, 0, 1)
#define Vector3I_Zero Vector3I_Create3(0, 0, 0)
#define Vector3I_One Vector3I_Create3(1, 1, 1)
#define Vector3I_MinusOne Vector3I_Create3(-1, -1, -1)

/* Adds a and b. */
void Vector3_Add(Vector3* result, Vector3* a, Vector3* b);
/* Adds a and b. */
void Vector3I_Add(Vector3I* result, Vector3I* a, Vector3I* b);
/* Adds b to all components of a. */
void Vector3_Add1(Vector3* result, Vector3* a, Real32 b);
/* Subtracts b from a. */
void Vector3_Subtract(Vector3* result, Vector3* a, Vector3* b);
/* Subtracts b from a. */
void Vector3I_Subtract(Vector3I* result, Vector3I* a, Vector3I* b);
/* Multiplies all components of a by scale. */
void Vector3_Multiply1(Vector3* result, Vector3* a, Real32 scale);
/* Multiplies all components of a by scale. */
void Vector3I_Multiply1(Vector3I* result, Vector3I* a, Int32 scale);
/* Multiplies components of a by scale. */
void Vector3_Multiply3(Vector3* result, Vector3* a, Vector3* scale);
/* Divides all components of a by scale. */
void Vector3_Divide1(Vector3* result, Vector3* a, Real32 scale);
/* Divides components of a by scale. */
void Vector3_Divide3(Vector3* result, Vector3* a, Vector3* scale);
/* Negates components of a. */
void Vector3_Negate(Vector3* result, Vector3* a);
/* Negates components of a. */
void Vector3I_Negate(Vector3I* result, Vector3I* a);

/* Linearly interpolates between two vectors. */
void Vector3_Lerp(Vector3* result, Vector3* a, Vector3* b, Real32 blend);
/* Calculates the dot product of two vectors. */
Real32 Vector3_Dot(Vector3* left, Vector3* right);
/* Calculates the cross product of two vectors. */
void Vector3_Cross(Vector3* result, Vector3* a, Vector3* b);
/* Calculates the normal of a vector. */
void Vector3_Normalize(Vector3* result, Vector3* a);

/* Transforms a vector by the given matrix. */
void Vector3_Transform(Vector3* result, Vector3* a, Matrix* mat);
/* Transforms a vector consisting of only a X component by the given matrix. */
void Vector3_TransformX(Vector3* result, Real32 x, Matrix* mat);
/* Transforms a vector consisting of only a Y component by the given matrix. */
void Vector3_TransformY(Vector3* result, Real32 y, Matrix* mat);
/* Transforms a vector consisting of only a Z component by the given matrix. */
void Vector3_TransformZ(Vector3* result, Real32 z, Matrix* mat);

/* Rotates the given 3D coordinates around the x axis. */
Vector3 Vector3_RotateX(Vector3 v, Real32 angle);
/* Rotates the given 3D coordinates around the y axis. */
Vector3 Vector3_RotateY(Vector3 v, Real32 angle);
/* Rotates the given 3D coordinates around the y axis. */
Vector3 Vector3_RotateY3(Real32 x, Real32 y, Real32 z, Real32 angle);
/* Rotates the given 3D coordinates around the z axis. */
Vector3 Vector3_RotateZ(Vector3 v, Real32 angle);

/* Returns whether the two vectors are exact same on all axes. */
bool Vector3_Equals(Vector3* a, Vector3* b);
/* Returns whether the two vectors are different on any axis. */
bool Vector3_NotEquals(Vector3* a, Vector3* b);
/* Returns whether the two vectors are exact same on all axes. */
bool Vector3I_Equals(Vector3I* a, Vector3I* b);
/* Returns whether the two vectors are different on any axes. */
bool Vector3I_NotEquals(Vector3I* a, Vector3I* b);

/* Returns a vector such that each component is floor of input floating-point component.*/
void Vector3I_Floor(Vector3I* result, Vector3* a);
/* Returns a vector with the integer components converted to floating-point components.*/
void Vector3I_ToVector3(Vector3* result, Vector3I* a);

/* Returns a vector such that each component is minimum of corresponding a and b component.*/
void Vector3I_Min(Vector3I* result, Vector3I* a, Vector3I* b);
/* Returns a vector such that each component is maximum of corresponding a and b component.*/
void Vector3I_Max(Vector3I* result, Vector3I* a, Vector3I* b);

/* Returns a normalised vector that faces in the direction described by the given yaw and pitch. */
Vector3 Vector3_GetDirVector(Real32 yawRad, Real32 pitchRad);
/* Returns the yaw and pitch of the given direction vector.
NOTE: This is not an identity function. Returned pitch is always within [-90, 90] degrees.*/
void Vector3_GetHeading(Vector3 dir, Real32* yawRad, Real32* pitchRad);
#endif