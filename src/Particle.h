#ifndef CC_PARTICLE_H
#define CC_PARTICLE_H
#include "Vectors.h"
#include "VertexStructs.h"
/* Represents particle effects, and manages rendering and spawning particles.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

struct IGameComponent;
struct ScheduledTask;
struct Particle {
	Vector3 Velocity;
	Real32 Lifetime;
	Vector3 LastPos, NextPos;
	UInt8 Size;
};

/* http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/ */
void Particle_DoRender(Vector2* size, Vector3* pos, TextureRec* rec, PackedCol col, VertexP3fT2fC4b* vertices);
void Particles_MakeComponent(struct IGameComponent* comp);
void Particles_Render(Real64 delta, Real32 t);
void Particles_Tick(struct ScheduledTask* task);
void Particles_BreakBlockEffect(Vector3I coords, BlockID oldBlock, BlockID block);
void Particles_RainSnowEffect(Vector3 pos);
#endif
