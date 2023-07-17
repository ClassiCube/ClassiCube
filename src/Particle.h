#ifndef CC_PARTICLE_H
#define CC_PARTICLE_H
#include "Vectors.h"
#include "PackedCol.h"
/* 
Represents particle effects, and manages rendering and spawning particles
Copyright 2014-2023 ClassiCube | Licensed under BSD-3
*/

struct IGameComponent;
struct VertexTextured;
struct ScheduledTask;
extern struct IGameComponent Particles_Component;

struct Particle {
	Vec3 velocity;
	float lifetime;
	Vec3 lastPos, nextPos;
	float size;
};

struct CustomParticleEffect {
	TextureRec rec;
	PackedCol tintCol;
	cc_uint8 frameCount;
	cc_uint8 particleCount;
	cc_uint8 collideFlags;
	cc_bool fullBright;
	float size;
	float sizeVariation;
	float spread; /* how far from the spawnpoint their location can vary */
	float speed; /* how fast they move away/towards the origin */
	float gravity;
	float baseLifetime; /* how long (in seconds) the particle lives for */
	float lifetimeVariation;
};

extern struct CustomParticleEffect Particles_CustomEffects[256];

/* http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/ */
void Particle_DoRender(const Vec2* size, const Vec3* pos, const TextureRec* rec, PackedCol col, struct VertexTextured* vertices);
void Particles_Render(float t);
void Particles_BreakBlockEffect(IVec3 coords, BlockID oldBlock, BlockID block);
void Particles_RainSnowEffect(float x, float y, float z);
void Particles_CustomEffect(int effectID, float x, float y, float z, float originX, float originY, float originZ);
#endif
