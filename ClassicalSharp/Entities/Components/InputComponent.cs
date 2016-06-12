// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp.Entities {
	
	/// <summary> Entity component that performs input handling. </summary>
	public sealed class InputComponent {
		
		Entity entity;
		Game game;
		public HacksComponent Hacks;
		public CollisionsComponent collisions;
		public PhysicsComponent physics;
		
		public InputComponent( Game game, Entity entity ) {
			this.game = game;
			this.entity = entity;
		}
		
		internal bool Handles( Key key ) {
			LocalPlayer p = (LocalPlayer)entity;
			KeyMap keys = game.InputHandler.Keys;
			
			if( key == keys[KeyBind.Respawn] && Hacks.CanRespawn ) {
				DoRespawn();
			} else if( key == keys[KeyBind.SetSpawn] && Hacks.CanRespawn ) {
				p.Spawn = entity.Position;
				p.Spawn.X = Utils.Floor( p.Spawn.X ) + 0.5f;
				p.Spawn.Z = Utils.Floor( p.Spawn.Z ) + 0.5f;
				p.SpawnYaw = entity.YawDegrees;
				p.SpawnPitch = entity.PitchDegrees;				
				DoRespawn();
			} else if( key == keys[KeyBind.Fly] && Hacks.CanFly && Hacks.Enabled ) {
				Hacks.Flying = !Hacks.Flying;
			} else if( key == keys[KeyBind.NoClip] && Hacks.CanNoclip && Hacks.Enabled ) {
				if( Hacks.Noclip ) entity.Velocity.Y = 0;
				Hacks.Noclip = !Hacks.Noclip;
			} else if( key == keys[KeyBind.Jump] && !entity.onGround && !(Hacks.Flying || Hacks.Noclip) ) {
				if( !physics.firstJump && Hacks.CanDoubleJump && Hacks.DoubleJump ) {
					physics.DoNormalJump();
					physics.firstJump = true;
				} else if( !physics.secondJump && Hacks.CanDoubleJump && Hacks.DoubleJump ) {
					physics.DoNormalJump();
					physics.secondJump = true;
				}
			} else {
				return false;
			}
			return true;
		}
		
		void DoRespawn() {
			LocalPlayer p = (LocalPlayer)entity;
			Vector3 spawn = p.Spawn;
			if( game.World.IsValidPos( Vector3I.Floor( spawn ) ) )
				FindHighestFree( ref spawn );
			
			spawn.Y += 2/16f;
			LocationUpdate update = LocationUpdate.MakePosAndOri( spawn, p.SpawnYaw, p.SpawnPitch, false );
			entity.SetLocation( update, false );
			entity.Velocity = Vector3.Zero;
			
			// Update onGround, otherwise if 'respawn' then 'space' is pressed, you still jump into the air if onGround was true before
			AABB bb = entity.CollisionBounds;
			bb.Min.Y -= 0.01f; bb.Max.Y = bb.Min.Y;
			entity.onGround = entity.TouchesAny( bb, b => game.BlockInfo.Collide[b] == CollideType.Solid );
		}
		
		void FindHighestFree( ref Vector3 spawn ) {
			BlockInfo info = game.BlockInfo;
			Vector3 size = entity.CollisionSize;
			AABB bb = AABB.Make( spawn, size );
			
			Vector3I P = Vector3I.Floor( spawn );
			int bbMax = Utils.Floor( size.Y );
			int minX = Utils.Floor( P.X - size.X / 2 ), maxX = Utils.Floor( P.X + size.X / 2 );
			int minZ = Utils.Floor( P.Z - size.Z / 2 ), maxZ = Utils.Floor( P.Z + size.Z / 2 );
			
			// Spawn player at highest valid position.
			for( int y = P.Y; y <= game.World.Height; y++ ) {
				bool intersectAny = false;
				for( int yy = 0; yy <= bbMax; yy++ )
					for( int z = minZ; z <= maxZ; z++ )
						for ( int x = minX; x <= maxX; x++ )
				{
					Vector3I coords = new Vector3I( x, y + yy, z );
					byte block = collisions.GetPhysicsBlockId( coords.X, coords.Y, coords.Z );
					Vector3 min = info.MinBB[block] + (Vector3)coords;
					Vector3 max = info.MaxBB[block] + (Vector3)coords;
					if( !bb.Intersects( new AABB( min, max ) ) ) continue;
					intersectAny |= info.Collide[block] == CollideType.Solid;
				}
				
				if( !intersectAny ) {
					byte block = collisions.GetPhysicsBlockId( P.X, y, P.Z );
					float height = info.Collide[block] == CollideType.Solid ? info.MaxBB[block].Y : 0;
					spawn.Y = y + height + Entity.Adjustment;
					return;
				}
				bb.Min.Y += 1; bb.Max.Y += 1;
			}
		}
	}
}