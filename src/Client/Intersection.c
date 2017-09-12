#include "Intersection.h"
#include "AABB.h"
#include "ExtMath.h"

Vector3 Intersection_InverseRotate(Vector3 pos, Entity* target) {
	pos = Vector3_RotateY(pos, -target->RotY * MATH_DEG2RAD);
	pos = Vector3_RotateZ(pos, -target->RotZ * MATH_DEG2RAD);
	pos = Vector3_RotateX(pos, -target->RotX * MATH_DEG2RAD);
	return pos;
}

bool Intersection_RayIntersectsRotatedBox(Vector3 origin, Vector3 dir, Entity* target, Real32* tMin, Real32* tMax) {
	/* This is the rotated AABB of the model we want to test for intersection
			  *
			 / \     we then perform a counter       *---*   and we can then do
	   ====>* x *    rotation to the rotated AABB    | x |   a standard AABB test
			 \ /     and ray origin and direction    *---*   with the rotated ray
			  *                                     /
												   /                           
	*/
	Vector3 delta; Vector3_Subtract(&delta, &origin, &target->Position); /* delta  = origin - target.Position */
	delta = Intersection_InverseRotate(delta, target);                   /* delta  = UndoRotation(delta) */
	Vector3_Add(&origin, &delta, &target->Position);                     /* origin = delta + target.Position */

	dir = Intersection_InverseRotate(dir, target);
	AABB bb;
	Entity_GetPickingBounds(target, &bb);
	return Intersection_RayIntersectsBox(origin, dir, bb.Min, bb.Max, tMin, tMax);
}

bool Intersection_RayIntersectsBox(Vector3 origin, Vector3 dir, Vector3 min, Vector3 max, Real32* t0, Real32* t1) {
	*t0 = 0; *t1 = 0;
	Real32 tmin, tmax, tymin, tymax, tzmin, tzmax;
	Real32 invDirX = 1.0f / dir.X;
	if (invDirX >= 0) {
		tmin = (min.X - origin.X) * invDirX;
		tmax = (max.X - origin.X) * invDirX;
	} else {
		tmin = (max.X - origin.X) * invDirX;
		tmax = (min.X - origin.X) * invDirX;
	}

	Real32 invDirY = 1.0f / dir.Y;
	if (invDirY >= 0) {
		tymin = (min.Y - origin.Y) * invDirY;
		tymax = (max.Y - origin.Y) * invDirY;
	} else {
		tymin = (max.Y - origin.Y) * invDirY;
		tymax = (min.Y - origin.Y) * invDirY;
	}
	if (tmin > tymax || tymin > tmax) return false;
	if (tymin > tmin) tmin = tymin;
	if (tymax < tmax) tmax = tymax;

	Real32 invDirZ = 1.0f / dir.Z;
	if (invDirZ >= 0) {
		tzmin = (min.Z - origin.Z) * invDirZ;
		tzmax = (max.Z - origin.Z) * invDirZ;
	} else {
		tzmin = (max.Z - origin.Z) * invDirZ;
		tzmax = (min.Z - origin.Z) * invDirZ;
	}

	if (tmin > tzmax || tzmin > tmax) return false;
	if (tzmin > tmin) tmin = tzmin;
	if (tzmax < tmax) tmax = tzmax;

	*t0 = tmin; *t1 = tmax;
	return *t0 >= 0;
}