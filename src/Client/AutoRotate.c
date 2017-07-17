#include "AutoRotate.h"
#include "Block.h"
#include "GameProps.h"
#include "LocationUpdate.h"
#include "LocalPlayer.h"
#include "Funcs.h"

#define AR_EQ1(s, x) (s.length >= 1 && Char_ToLower(s.buffer[0]) == x)
#define AR_EQ2(s, x, y) (s.length >= 2 && Char_ToLower(s.buffer[0]) == x && Char_ToLower(s.buffer[1]) == y)

BlockID AutoRotate_RotateBlock(BlockID block) {
	String name = Block_Name[block];
	Int32 dirIndex = String_LastIndexOf(&name, '-');
	if (dirIndex == -1) return block; /* not a directional block */

	String dir = name.Substring(dirIndex + 1);
	name = name.Substring(0, dirIndex);

	Vector3 translated, offset;
	Vector3I_ToVector3(&translated, &Game_SelectedPos.TranslatedPos);
	Vector3_Subtract(&offset, &Game_SelectedPos.Intersect, &translated);

	if (AR_EQ2(dir, 'n','w') || AR_EQ2(dir, 'n','e') || AR_EQ2(dir, 's','w') || AR_EQ2(dir, 's','e')) {
		return AutoRotate_RotateCorner(block, &name, offset);
	} else if (AR_EQ1(dir, 'u') || AR_EQ1(dir, 'd')) {
		return AutoRotate_RotateVertical(block, &name, offset);
	} else if (AR_EQ1(dir, 'n') || AR_EQ1(dir, 'w') || AR_EQ1(dir, 's') || AR_EQ1(dir, 'e')) {
		return AutoRotate_RotateDirection(block, &name, offset);
	} else if (AR_EQ2(dir, 'u','d') || AR_EQ2(dir, 'w','e') || AR_EQ2(dir, 'n','s')) {
		return AutoRotate_RotateOther(block, &name, offset);
	}
	return block;
}


BlockID AutoRotate_RotateCorner(BlockID block, String* name, Vector3 offset) {
	if (offset.X < 0.5f && offset.Z < 0.5f) {
		return AutoRotate_Find(block, name, "-NW");
	} else if (offset.X >= 0.5f && offset.Z < 0.5f) {
		return AutoRotate_Find(block, name, "-NE");
	} else if (offset.X < 0.5f && offset.Z >= 0.5f) {
		return AutoRotate_Find(block, name, "-SW");
	} else if (offset.X >= 0.5f && offset.Z >= 0.5f) {
		return AutoRotate_Find(block, name, "-SE");
	}
	return block;
}

BlockID AutoRotate_RotateVertical(BlockID block, String* name, Vector3 offset) {
	if (offset.Y >= 0.5f) {
		return AutoRotate_Find(block, name, "-U");
	} else {
		return AutoRotate_Find(block, name, "-D");
	}
}

BlockID AutoRotate_RotateOther(BlockID block, String* name, Vector3 offset) {
	/* Fence type blocks */
	if (AutoRotate_Find(BlockID_Invalid, name, "-UD") == BlockID_Invalid) {
		Real32 headY = LocalPlayer_Instance.Base.Base.HeadY;
		headY = LocationUpdate_Clamp(headY);

		if (headY < 45.0f || (headY >= 135.0f && headY < 225.0f) || headY > 315.0f) {
			return AutoRotate_Find(block, name, "-WE");
		} else {
			return AutoRotate_Find(block, name, "-NS");
		}
	}

	/* Thin pillar type blocks */
	Face face = Game_SelectedPos.ClosestFace;
	if (face == Face_YMax || face == Face_YMin)
		return AutoRotate_Find(block, name, "-UD");
	if (face == Face_XMax || face == Face_XMin)
		return AutoRotate_Find(block, name, "-WE");
	if (face == Face_ZMax || face == Face_ZMin)
		return AutoRotate_Find(block, name, "-NS");
	return block;
}

BlockID AutoRotate_RotateDirection(BlockID block, String* name, Vector3 offset) {
	Vector3 SE = Vector3_Create3(+1.0f, 0.0f, 1.0f);
	Vector3 SW = Vector3_Create3(-1.0f, 0.0f, 1.0f);

	Vector3I pos = Game_SelectedPos.TranslatedPos;
	Vector3 exact = Game_SelectedPos.Intersect;
	Vector3 exactFlat = exact; exactFlat.Y = 0.0f;

	Vector3 SEToPoint = exactFlat; SEToPoint.X -= pos.X; SEToPoint.Z -= pos.Z;
	Vector3 SWToPoint = exactFlat; SWToPoint.X -= (pos.X + 1); SWToPoint.Z -= pos.Z;

	Real32 dotSE = Vector3_Dot(&SEToPoint, &SW);
	Real32 dotSW = Vector3_Dot(&SWToPoint, &SE);

	if (dotSE <= 0.0f) { /* NorthEast */
		if (dotSW <= 0.0f) { /* NorthWest */
			return AutoRotate_Find(block, name, "-N");
		} else { /* SouthEast */
			return AutoRotate_Find(block, name, "-E");
		}
	} else { /* SouthWest */
		if (dotSW <= 0.0f) { /* NorthWest */
			return AutoRotate_Find(block, name, "-W");
		} else { /* SouthEast */
			return AutoRotate_Find(block, name, "-S");
		}
	}
}

static BlockID AutoRotate_Find(BlockID block, String* name, const UInt8* suffix) {
	UInt8 buffer[String_BufferSize(128)];
	String temp = String_FromRawBuffer(buffer, 128);
	String_AppendString(&temp, name);
	String_AppendConstant(&temp, suffix);

	Int32 rotated = Block_FindID(&temp);
	if (rotated != -1) return (BlockID)rotated;
	return block;
}