#include "Picking.h"
#include "ExtMath.h"
#include "Game.h"
#include "Physics.h"
#include "Entity.h"
#include "World.h"
#include "Funcs.h"
#include "Block.h"
#include "Logger.h"
#include "Camera.h"

static float pickedPos_dist;
static void TestAxis(struct RayTracer* t, float dAxis, Face fAxis) {
	dAxis = Math_AbsF(dAxis);
	if (dAxis >= pickedPos_dist) return;

	pickedPos_dist = dAxis;
	t->Closest     = fAxis;
}

static void SetAsValid(struct RayTracer* t) {
	t->TranslatedPos = t->pos;
	t->Valid         = true;

	pickedPos_dist = MATH_LARGENUM;
	TestAxis(t, t->Intersect.X - t->Min.X, FACE_XMIN);
	TestAxis(t, t->Intersect.X - t->Max.X, FACE_XMAX);
	TestAxis(t, t->Intersect.Y - t->Min.Y, FACE_YMIN);
	TestAxis(t, t->Intersect.Y - t->Max.Y, FACE_YMAX);
	TestAxis(t, t->Intersect.Z - t->Min.Z, FACE_ZMIN);
	TestAxis(t, t->Intersect.Z - t->Max.Z, FACE_ZMAX);

	switch (t->Closest) {
	case FACE_XMIN: t->TranslatedPos.X--; break;
	case FACE_XMAX: t->TranslatedPos.X++; break;
	case FACE_ZMIN: t->TranslatedPos.Z--; break;
	case FACE_ZMAX: t->TranslatedPos.Z++; break;
	case FACE_YMIN: t->TranslatedPos.Y--; break;
	case FACE_YMAX: t->TranslatedPos.Y++; break;
	}
}

void RayTracer_SetInvalid(struct RayTracer* t) {
	static const IVec3 pos = { -1, -1, -1 };
	t->pos           = pos;
	t->TranslatedPos = pos;

	t->Valid   = false;
	t->block   = BLOCK_AIR;
	t->Closest = FACE_COUNT;
}

static float RayTracer_Div(float a, float b) {
	if (Math_AbsF(b) < 0.000001f) return MATH_LARGENUM;
	return a / b;
}

void RayTracer_Init(struct RayTracer* t, const Vec3* origin, const Vec3* dir) {
	IVec3 cellBoundary;
	t->origin = *origin; t->dir = *dir;

	/* Rounds the position's X, Y and Z down to the nearest integer values. */
	/* The cell in which the ray starts. */
	IVec3_Floor(&t->pos, origin);
	/* Determine which way we go. */
	t->step.X = Math_Sign(dir->X); t->step.Y = Math_Sign(dir->Y); t->step.Z = Math_Sign(dir->Z);

	/* Calculate cell boundaries. When the step (i.e. direction sign) is positive,
	the next boundary is AFTER our current position, meaning that we have to add 1.
	Otherwise, it is BEFORE our current position, in which case we add nothing. */
	cellBoundary.X = t->pos.X + (t->step.X > 0 ? 1 : 0);
	cellBoundary.Y = t->pos.Y + (t->step.Y > 0 ? 1 : 0);
	cellBoundary.Z = t->pos.Z + (t->step.Z > 0 ? 1 : 0);

	/* NOTE: we want it so if dir.x = 0, tmax.x = positive infinity
	Determine how far we can travel along the ray before we hit a voxel boundary. */
	t->tMax.X = RayTracer_Div(cellBoundary.X - origin->X, dir->X); /* Boundary is a plane on the YZ axis. */
	t->tMax.Y = RayTracer_Div(cellBoundary.Y - origin->Y, dir->Y); /* Boundary is a plane on the XZ axis. */
	t->tMax.Z = RayTracer_Div(cellBoundary.Z - origin->Z, dir->Z); /* Boundary is a plane on the XY axis. */

	/* Determine how far we must travel along the ray before we have crossed a gridcell. */
	t->tDelta.X = RayTracer_Div((float)t->step.X, dir->X);
	t->tDelta.Y = RayTracer_Div((float)t->step.Y, dir->Y);
	t->tDelta.Z = RayTracer_Div((float)t->step.Z, dir->Z);
}

void RayTracer_Step(struct RayTracer* t) {
	/* For each step, determine which distance to the next voxel boundary is lowest
	(i.e. which voxel boundary is nearest) and walk that way. */
	if (t->tMax.X < t->tMax.Y && t->tMax.X < t->tMax.Z) {
		/* tMax.X is the lowest, an YZ cell boundary plane is nearest. */
		t->pos.X  += t->step.X;
		t->tMax.X += t->tDelta.X;
	} else if (t->tMax.Y < t->tMax.Z) {
		/* tMax.Y is the lowest, an XZ cell boundary plane is nearest. */
		t->pos.Y  += t->step.Y;
		t->tMax.Y += t->tDelta.Y;
	} else {
		/* tMax.Z is the lowest, an XY cell boundary plane is nearest. */
		t->pos.Z  += t->step.Z;
		t->tMax.Z += t->tDelta.Z;
	}
}

#define PICKING_BORDER BLOCK_BEDROCK
typedef cc_bool (*IntersectTest)(struct RayTracer* t);

static BlockID Picking_GetInside(int x, int y, int z) {
	cc_bool sides;
	int height;

	if (World_ContainsXZ(x, z)) {
		if (y >= World.Height) return BLOCK_AIR;
		if (y >= 0) return World_GetBlock(x, y, z);
	}

	/* bedrock on bottom or outside map */
	sides  = Env.SidesBlock != BLOCK_AIR;
	height = Env_SidesHeight; if (height < 1) height = 1;
	return sides && y < height ? PICKING_BORDER : BLOCK_AIR;
}

static BlockID Picking_GetOutside(int x, int y, int z, IVec3 origin) {
	cc_bool sides;
	int height;

	if (!World_ContainsXZ(x, z)) return BLOCK_AIR;
	sides = Env.SidesBlock != BLOCK_AIR;
	/* handling of blocks inside the map, above, and on borders */

	if (y >= World.Height) return BLOCK_AIR;
	if (sides && y == -1 && origin.Y > 0) return PICKING_BORDER;
	if (sides && y ==  0 && origin.Y < 0) return PICKING_BORDER;
	height = Env_SidesHeight; if (height < 1) height = 1;

	if (sides && x == 0          && y >= 0 && y < height && origin.X < 0)             return PICKING_BORDER;
	if (sides && z == 0          && y >= 0 && y < height && origin.Z < 0)             return PICKING_BORDER;
	if (sides && x == World.MaxX && y >= 0 && y < height && origin.X >= World.Width)  return PICKING_BORDER;
	if (sides && z == World.MaxZ && y >= 0 && y < height && origin.Z >= World.Length) return PICKING_BORDER;
	if (y >= 0) return World_GetBlock(x, y, z);
	return BLOCK_AIR;
}

static cc_bool RayTrace(struct RayTracer* t, const Vec3* origin, const Vec3* dir, float reach, IntersectTest intersect) {
	IVec3 pOrigin;
	cc_bool insideMap;
	float reachSq;
	Vec3 v, minBB, maxBB;

	float dxMin, dxMax, dx;
	float dyMin, dyMax, dy;
	float dzMin, dzMax, dz;
	int i, x, y, z;

	RayTracer_Init(t, origin, dir);
	IVec3_Floor(&pOrigin, origin);
	insideMap = World_Contains(pOrigin.X, pOrigin.Y, pOrigin.Z);
	reachSq   = reach * reach;
		
	for (i = 0; i < 25000; i++) {
		x   = t->pos.X; y   = t->pos.Y; z   = t->pos.Z;
		v.X = (float)x; v.Y = (float)y; v.Z = (float)z;

		t->block = insideMap ? Picking_GetInside(x, y, z) : Picking_GetOutside(x, y, z, pOrigin);
		Vec3_Add(&t->Min, &v, &Blocks.RenderMinBB[t->block]);
		Vec3_Add(&t->Max, &v, &Blocks.RenderMaxBB[t->block]);

		dxMin = Math_AbsF(origin->X - t->Min.X); dxMax = Math_AbsF(origin->X - t->Max.X);
		dyMin = Math_AbsF(origin->Y - t->Min.Y); dyMax = Math_AbsF(origin->Y - t->Max.Y);
		dzMin = Math_AbsF(origin->Z - t->Min.Z); dzMax = Math_AbsF(origin->Z - t->Max.Z);
		dx = min(dxMin, dxMax); dy = min(dyMin, dyMax); dz = min(dzMin, dzMax);
		if (dx * dx + dy * dy + dz * dz > reachSq) return false;

		if (intersect(t)) return true;
		RayTracer_Step(t);
	}

	Logger_Abort("Something went wrong, did over 25,000 iterations in Picking_RayTrace()");
	return false;
}

static cc_bool ClipBlock(struct RayTracer* t) {
	Vec3 scaledDir;
	float lenSq, reach;
	float t0, t1;

	if (!Game_CanPick(t->block)) return false;
	/* This cell falls on the path of the ray. Now perform an additional AABB test,
	since some blocks do not occupy a whole cell. */
	if (!Intersection_RayIntersectsBox(t->origin, t->dir, t->Min, t->Max, &t0, &t1)) return false;
	
	Vec3_Mul1(&scaledDir, &t->dir, t0);              /* scaledDir = dir * t0 */
	Vec3_Add(&t->Intersect, &t->origin, &scaledDir); /* intersect = origin + scaledDir */

	/* Only pick the block if the block is precisely within reach distance. */
	lenSq = Vec3_LengthSquared(&scaledDir);
	reach = LocalPlayer_Instance.ReachDistance;

	if (lenSq <= reach * reach) {
		SetAsValid(t);
	} else {
		RayTracer_SetInvalid(t);
	}
	return true;
}

const static Vec3 picking_adjust = { 0.1f, 0.1f, 0.1f };
static cc_bool ClipCamera(struct RayTracer* t) {
	Vec3 intersect;
	float t0, t1;

	if (Blocks.Draw[t->block] == DRAW_GAS || Blocks.Collide[t->block] != COLLIDE_SOLID) return false;
	if (!Intersection_RayIntersectsBox(t->origin, t->dir, t->Min, t->Max, &t0, &t1)) return false;

	/* Need to collide with slightly outside block, to avoid camera clipping issues */
	Vec3_Sub(&t->Min, &t->Min, &picking_adjust);
	Vec3_Add(&t->Max, &t->Max, &picking_adjust);
	Intersection_RayIntersectsBox(t->origin, t->dir, t->Min, t->Max, &t0, &t1);
	
	Vec3_Mul1(&intersect,   &t->dir, t0);            /* intersect = dir * t0 */
	Vec3_Add(&t->Intersect, &t->origin, &intersect); /* intersect = origin + dir * t0 */
	SetAsValid(t);
	return true;
}

void Picking_CalcPickedBlock(const Vec3* origin, const Vec3* dir, float reach, struct RayTracer* t) {
	if (!RayTrace(t, origin, dir, reach, ClipBlock)) {
		RayTracer_SetInvalid(t);
	}
}

void Picking_ClipCameraPos(const Vec3* origin, const Vec3* dir, float reach, struct RayTracer* t) {
	cc_bool noClip = !Camera.Clipping || LocalPlayer_Instance.Hacks.Noclip;
	if (noClip || !RayTrace(t, origin, dir, reach, ClipCamera)) {
		RayTracer_SetInvalid(t);
		Vec3_Mul1(&t->Intersect, dir, reach);           /* intersect = dir * reach */
		Vec3_Add(&t->Intersect, origin, &t->Intersect); /* intersect = origin + dir * reach */
	}
}
