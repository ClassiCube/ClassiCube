#ifndef CS_LOCATIONUPDATE_H
#define CS_LOCATIONUPDATE_H
#include "Typedefs.h"
#include "Vectors.h"
/* Represents a location (position and/or orientation) update for an entity.
Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

/* Stores data that describes either a relative position,
full position, or an orientation update for an entity. */
typedef struct LocationUpdate {
	/* Position of the update (if included). */
	Vector3 Pos;
	/* Orientation of the update (if included). If not, has the value of LocationUpdate_Excluded. */
	Real32 RotX, RotY, RotZ, HeadX;
	/* Whether this update includes an absolute or relative position. */
	bool IncludesPosition;
	/* Whether the positon is absolute, or relative to the last positionreceived from the server. */
	bool RelativePosition;
} LocationUpdate;


/* Constant value specifying an angle is not included in an orientation update. */
#define LocationUpdate_Excluded -100000.31415926535f

/* Clamps the given angle so it lies between [0, 360). */
Real32 LocationUpdate_Clamp(Real32 degrees);


/* Constructs a location update with values for every field.
You should generally prefer using the alternative constructors. */
void LocationUpdate_Construct(LocationUpdate* update, Real32 x, Real32 y, Real32 z,
	Real32 rotX, Real32 rotY, Real32 rotZ, Real32 headX, bool incPos, bool relPos);

/* Constructs a location update that does not have any position or orientation information. */
void LocationUpdate_Empty(LocationUpdate* update);

/* Constructs a location update that only consists of orientation information. */
void LocationUpdate_MakeOri(LocationUpdate* update, Real32 rotY, Real32 headX);

/* Constructs a location update that only consists of position information. */
void LocationUpdate_MakePos(LocationUpdate* update, Vector3 pos, bool rel);

/* Constructs a location update that consists of position and orientation information. */
void LocationUpdate_MakePosAndOri(LocationUpdate* update, Vector3 pos,
	Real32 rotY, Real32 headX, bool rel);
#endif