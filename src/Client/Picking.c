#include "Picking.h"
#include "ExtMath.h"
#include "Game.h"
#include "Intersection.h"
#include "Player.h"
#include "World.h"
#include "Funcs.h"

Real32 PickedPos_dist;
void PickedPos_TestAxis(PickedPos* pos, Real32 dAxis, Face fAxis) {
	dAxis = Math_AbsF(dAxis);
	if (dAxis >= PickedPos_dist) return;

	PickedPos_dist = dAxis;
	pos->ClosestFace = fAxis;
}

void PickedPos_SetAsValid(PickedPos* pos, RayTracer* t, Vector3 intersect) {
	pos->Min = t->Min; pos->Max = t->Max;
	pos->BlockPos = Vector3I_Create3(t->X, t->Y, t->Z);
	pos->Valid = true;
	pos->Block = t->Block;
	pos->Intersect = intersect;

	PickedPos_dist = MATH_LARGENUM;
	PickedPos_TestAxis(pos, intersect.X - t->Min.X, Face_XMin);
	PickedPos_TestAxis(pos, intersect.X - t->Max.X, Face_XMax);
	PickedPos_TestAxis(pos, intersect.Y - t->Min.Y, Face_YMin);
	PickedPos_TestAxis(pos, intersect.Y - t->Max.Y, Face_YMax);
	PickedPos_TestAxis(pos, intersect.Z - t->Min.Z, Face_ZMin);
	PickedPos_TestAxis(pos, intersect.Z - t->Max.Z, Face_ZMax);

	Vector3I offsets[Face_Count];
	offsets[Face_XMin] = Vector3I_Create3(-1, 0, 0);
	offsets[Face_XMax] = Vector3I_Create3(+1, 0, 0);
	offsets[Face_ZMin] = Vector3I_Create3(0, 0, -1);
	offsets[Face_ZMax] = Vector3I_Create3(0, 0, +1);
	offsets[Face_YMin] = Vector3I_Create3(0, -1, 0);
	offsets[Face_YMax] = Vector3I_Create3(0, +1, 0);
	Vector3I_Add(&pos->TranslatedPos, &pos->BlockPos, &offsets[pos->ClosestFace]);
}

void PickedPos_SetAsInvalid(PickedPos* pos) {
	pos->Valid = false;
	pos->BlockPos = Vector3I_MinusOne;
	pos->TranslatedPos = Vector3I_MinusOne;
	pos->ClosestFace = (Face)Face_Count;
	pos->Block = BlockID_Air;
}

Real32 RayTracer_Div(Real32 a, Real32 b) {
	if (Math_AbsF(b) < 0.000001f) return MATH_LARGENUM;
	return a / b;
}

void RayTracer_SetVectors(RayTracer* t, Vector3 origin, Vector3 dir) {
	t->Origin = origin; t->Dir = dir;
	/* Rounds the position's X, Y and Z down to the nearest integer values. */
	Vector3I start;
	Vector3I_Floor(&start, &origin);
	/* The cell in which the ray starts. */
	t->X = start.X; t->Y = start.Y; t->Z = start.Z;
	/* Determine which way we go.*/
	t->step.X = Math_Sign(dir.X); t->step.Y = Math_Sign(dir.Y); t->step.Z = Math_Sign(dir.Z);

	/* Calculate cell boundaries. When the step (i.e. direction sign) is positive,
	the next boundary is AFTER our current position, meaning that we have to add 1.
	Otherwise, it is BEFORE our current position, in which case we add nothing. */
	Vector3I cellBoundary;
	cellBoundary.X = start.X + (t->step.X > 0 ? 1 : 0);
	cellBoundary.Y = start.Y + (t->step.Y > 0 ? 1 : 0);
	cellBoundary.Z = start.Z + (t->step.Z > 0 ? 1 : 0);

	/* NOTE: we want it so if dir.x = 0, tmax.x = positive infinity
	Determine how far we can travel along the ray before we hit a voxel boundary. */
	t->tMax.X = RayTracer_Div(cellBoundary.X - origin.X, dir.X); /* Boundary is a plane on the YZ axis. */
	t->tMax.Y = RayTracer_Div(cellBoundary.Y - origin.Y, dir.Y); /* Boundary is a plane on the XZ axis. */
	t->tMax.Z = RayTracer_Div(cellBoundary.Z - origin.Z, dir.Z); /* Boundary is a plane on the XY axis. */

																 // Determine how far we must travel along the ray before we have crossed a gridcell.
	t->tDelta.X = RayTracer_Div((Real32)t->step.X, dir.X);
	t->tDelta.Y = RayTracer_Div((Real32)t->step.Y, dir.Y);
	t->tDelta.Z = RayTracer_Div((Real32)t->step.Z, dir.Z);
}

void RayTracer_Step(RayTracer* t) {
	/* For each step, determine which distance to the next voxel boundary is lowest
	(i.e. which voxel boundary is nearest) and walk that way. */
	if (t->tMax.X < t->tMax.Y && t->tMax.X < t->tMax.Z) {
		/* tMax.X is the lowest, an YZ cell boundary plane is nearest. */
		t->X += t->step.X;
		t->tMax.X += t->tDelta.X;
	} else if (t->tMax.Y < t->tMax.Z) {
		/* tMax.Y is the lowest, an XZ cell boundary plane is nearest. */
		t->Y += t->step.Y;
		t->tMax.Y += t->tDelta.Y;
	} else {
		/* tMax.Z is the lowest, an XY cell boundary plane is nearest. */
		t->Z += t->step.Z;
		t->tMax.Z += t->tDelta.Z;
	}
}

RayTracer tracer;
#define PICKING_BORDER BlockID_Bedrock
typedef bool(*IntersectTest)(PickedPos* pos);

BlockID Picking_GetBlock(Int32 x, Int32 y, Int32 z, Vector3I origin) {
	bool sides = WorldEnv_SidesBlock != BlockID_Air;
	Int32 height = WorldEnv_SidesHeight;
	if (height < 1) height = 1;
	bool insideMap = World_IsValidPos_3I(origin);

	/* handling of blocks inside the map, above, and on borders */
	if (x >= 0 && z >= 0 && x < World_Width && z < World_Length) {
		if (y >= World_Height) return 0;
		if (sides && y == -1 && origin.Y > 0) return PICKING_BORDER;
		if (sides && y == 0  && origin.Y < 0) return PICKING_BORDER;

		if (sides && x == 0 && y >= 0 && y < height && origin.X < 0) return PICKING_BORDER;
		if (sides && z == 0 && y >= 0 && y < height && origin.Z < 0) return PICKING_BORDER;
		if (sides && x == World_MaxX && y >= 0 && y < height && origin.X >= World_Width)
			return PICKING_BORDER;
		if (sides && z == World_MaxZ && y >= 0 && y < height && origin.Z >= World_Length)
			return PICKING_BORDER;
		if (y >= 0) return World_GetBlock(x, y, z);
	}

	/* pick blocks on the map boundaries (when inside the map) */
	if (!sides || !insideMap)   return BlockID_Air;
	if (y == 0 && origin.Y < 0) return PICKING_BORDER;

	bool validX = (x == -1 || x == World_Width) && (z >= 0 && z < World_Length);
	bool validZ = (z == -1 || z == World_Length) && (x >= 0 && x < World_Width);
	if (y >= 0 && y < height && (validX || validZ)) return PICKING_BORDER;
	return BlockID_Air;
}

bool Picking_RayTrace(Vector3 origin, Vector3 dir, Real32 reach, PickedPos* pos, IntersectTest intersect) {
	RayTracer_SetVectors(&tracer, origin, dir);
	Real32 reachSq = reach * reach;
	Vector3I pOrigin;
	Vector3I_Floor(&pOrigin, &origin);

	Int32 i;
	Vector3 blockPos;
	for (i = 0; i < 10000; i++) {
		Int32 x = tracer.X, y = tracer.Y, z = tracer.Z;
		blockPos.X = (Real32)x; blockPos.Y = (Real32)y; blockPos.Z = (Real32)z;
		tracer.Block = Picking_GetBlock(x, y, z, pOrigin);

		Vector3 minPos, maxPos;
		Vector3_Add(&minPos, &blockPos, &Block_RenderMinBB[tracer.Block]);
		Vector3_Add(&maxPos, &blockPos, &Block_RenderMaxBB[tracer.Block]);

		Real32 dxMin = Math_AbsF(origin.X - minPos.X), dxMax = Math_AbsF(origin.X - maxPos.X);
		Real32 dyMin = Math_AbsF(origin.Y - minPos.Y), dyMax = Math_AbsF(origin.Y - maxPos.Y);
		Real32 dzMin = Math_AbsF(origin.Z - minPos.Z), dzMax = Math_AbsF(origin.Z - maxPos.Z);
		Real32 dx = min(dxMin, dxMax), dy = min(dyMin, dyMax), dz = min(dzMin, dzMax);
		if (dx * dx + dy * dy + dz * dz > reachSq) return false;

		tracer.Min = minPos; tracer.Max = maxPos;
		if (intersect(pos)) return true;
		RayTracer_Step(&tracer);
	}

	ErrorHandler_Fail("Something went wrong, did over 10,000 iterations in Picking_RayTrace()");
	return false;
}

bool Picking_ClipBlock(PickedPos* pos) {
	if (!Game_CanPick(tracer.Block)) return false;

	/* This cell falls on the path of the ray. Now perform an additional AABB test,
	since some blocks do not occupy a whole cell. */
	Real32 t0, t1;
	if (!Intersection_RayIntersectsBox(tracer.Origin, tracer.Dir, tracer.Min, tracer.Max, &t0, &t1)) {
		return false;
	}

	Vector3 scaledDir, intersect;
	Vector3_Multiply1(&scaledDir, &tracer.Dir, t0);      /* scaledDir = dir * t0 */
	Vector3_Add(&intersect, &tracer.Origin, &scaledDir); /* intersect = origin + scaledDir */
														 /* Only pick the block if the block is precisely within reach distance. */
	Real32 lenSq = Vector3_LengthSquared(&scaledDir);
	Real32 reach = LocalPlayer_Instance.ReachDistance;

	if (lenSq <= reach * reach) {
		PickedPos_SetAsValid(pos, &tracer, intersect);
	}
	else {
		PickedPos_SetAsInvalid(pos);
	}
	return true;
}

bool Picking_ClipCamera(PickedPos* pos) {
	if (Block_Draw[tracer.Block] == DrawType_Gas || Block_Collide[tracer.Block] != CollideType_Solid) {
		return false;
	}

	Real32 t0, t1;
	if (!Intersection_RayIntersectsBox(tracer.Origin, tracer.Dir, tracer.Min, tracer.Max, &t0, &t1)) {
		return false;
	}

	Vector3 intersect;
	Vector3_Multiply1(&intersect, &tracer.Dir, t0);      /* intersect = dir * t0 */
	Vector3_Add(&intersect, &tracer.Origin, &intersect); /* intersect = origin + dir * t0 */
	PickedPos_SetAsValid(pos, &tracer, intersect);

#define PICKING_ADJUST 0.1f
	switch (pos->ClosestFace) {
	case Face_XMin: pos->Intersect.X -= PICKING_ADJUST; break;
	case Face_XMax: pos->Intersect.X += PICKING_ADJUST; break;
	case Face_YMin: pos->Intersect.Y -= PICKING_ADJUST; break;
	case Face_YMax: pos->Intersect.Y += PICKING_ADJUST; break;
	case Face_ZMin: pos->Intersect.Z -= PICKING_ADJUST; break;
	case Face_ZMax: pos->Intersect.Z += PICKING_ADJUST; break;
	}
	return true;
}

void Picking_CalculatePickedBlock(Vector3 origin, Vector3 dir, Real32 reach, PickedPos* pos) {
	if (!Picking_RayTrace(origin, dir, reach, pos, Picking_ClipBlock)) {
		PickedPos_SetAsInvalid(pos);
	}
}

void Picking_ClipCameraPos(Vector3 origin, Vector3 dir, Real32 reach, PickedPos* pos) {
	bool noClip = !Game_CameraClipping || LocalPlayer_Instance.Hacks.Noclip;
	if (noClip || !Picking_RayTrace(origin, dir, reach, pos, Picking_ClipCamera)) {
		PickedPos_SetAsInvalid(pos);
		Vector3_Multiply1(&pos->Intersect, &tracer.Dir, reach);        /* intersect = dir * reach */
		Vector3_Add(&pos->Intersect, &tracer.Origin, &pos->Intersect); /* intersect = origin + dir * reach */
	}
}