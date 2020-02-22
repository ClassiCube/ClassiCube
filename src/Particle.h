#ifndef CC_PARTICLE_H
#define CC_PARTICLE_H
#include "Vectors.h"
#include "VertexStructs.h"
/* Represents particle effects, and manages rendering and spawning particles.
   Copyright 2014-2019 ClassiCube | Licensed under BSD-3
*/

struct IGameComponent;
struct ScheduledTask;
extern struct IGameComponent Particles_Component;

struct Particle {
	Vec3 velocity;
	float lifetime;
	Vec3 lastPos, nextPos;
	float size;
};

struct CustomParticleProperty {
	TextureRec rec;
	int frameCount;
	int particleCount; //how many of this particle are spawned per spawn-packet
	float size; //size of the particle in fixed-point world units (e.g. 32 is a full block's length)
	float sizeVariation;
	float spread; //how far from the spawnpoint their location can vary (in fixed-point world units)
	float speed; //how fast they move away/towards the origin
	float gravity;
	float baseLifetime; //how long (in seconds) the particle lives for
	float lifetimeVariation;
	cc_bool expireUponTouchingGround;
	cc_bool fullBright;
};

extern struct CustomParticleProperty customParticle_properties[256];

/* http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/ */
void Particle_DoRender(const Vec2* size, const Vec3* pos, const TextureRec* rec, PackedCol col, VertexP3fT2fC4b* vertices);
void Particles_Render(float t);
void Particles_Tick(struct ScheduledTask* task);
void Particles_BreakBlockEffect(IVec3 coords, BlockID oldBlock, BlockID block);
void Particles_RainSnowEffect(float x, float y, float z);
void Particles_CustomEffect(int propertyID, float x, float y, float z, float originX, float originY, float originZ);
#endif
