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
		
		internal bool HandleKeyDown( Key key ) {
			LocalPlayer p = (LocalPlayer)entity;
			KeyMap keys = game.InputHandler.Keys;
			
			if( key == keys[KeyBinding.Respawn] && Hacks.CanRespawn ) {
				Vector3 spawn = p.Spawn;
				if( game.World.IsValidPos( Vector3I.Floor( spawn ) ) )
					FindHighestFree( ref spawn );
				
				spawn.Y += 1/16f;
				LocationUpdate update = LocationUpdate.MakePosAndOri( spawn, p.SpawnYaw, p.SpawnPitch, false );
				entity.SetLocation( update, false );
			} else if( key == keys[KeyBinding.SetSpawn] && Hacks.CanRespawn ) {
				p.Spawn = entity.Position;
				p.SpawnYaw = entity.YawDegrees;
				p.SpawnPitch = entity.PitchDegrees;
				
				Vector3 spawn = p.Spawn; spawn.Y += 1/16f;
				LocationUpdate update = LocationUpdate.MakePosAndOri( spawn, p.SpawnYaw, p.SpawnPitch, false );
				entity.SetLocation( update, false );
			} else if( key == keys[KeyBinding.Fly] && Hacks.CanFly && Hacks.Enabled ) {
				Hacks.Flying = !Hacks.Flying;
			} else if( key == keys[KeyBinding.NoClip] && Hacks.CanNoclip && Hacks.Enabled ) {
				if( Hacks.Noclip ) entity.Velocity.Y = 0;
				Hacks.Noclip = !Hacks.Noclip;
			} else if( key == keys[KeyBinding.Jump] && !entity.onGround && !(Hacks.Flying || Hacks.Noclip) ) {
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
		
		void FindHighestFree( ref Vector3 spawn ) {
			BlockInfo info = game.BlockInfo;
			Vector3 size = entity.Model.CollisionSize;
			BoundingBox bb = entity.CollisionBounds;
			Vector3I P = Vector3I.Floor( spawn );
			int bbMax = Utils.Floor( size.Y );			
			int minX = Utils.Floor( -size.X / 2 ), maxX = Utils.Floor( size.X / 2 );
			int minZ = Utils.Floor( -size.Z / 2 ), maxZ = Utils.Floor( size.Z / 2 );
			
			// Spawn player at highest valid position.
			for( int y = P.Y; y <= game.World.Height; y++ ) {
				bool anyHit = false;
				for( int yy = 0; yy <= bbMax; yy++ )
					for( int zz = minZ; zz <= maxZ; zz++ )
						for ( int xx = minX; xx <= maxX; xx++ )							
				{
					Vector3I coords = new Vector3I( P.X + xx, y + yy, P.Z + zz );
					byte block = collisions.GetPhysicsBlockId( coords.X, coords.Y, coords.Z );
					Vector3 min = info.MinBB[block] + (Vector3)coords;
					Vector3 max = info.MaxBB[block] + (Vector3)coords;
					if( !bb.Intersects( new BoundingBox( min, max ) ) ) continue;
					anyHit |= info.Collide[block] == CollideType.Solid;
				}
				
				if( !anyHit ) {
					byte block = collisions.GetPhysicsBlockId( P.X, y, P.Z );
					float height = info.Collide[block] == CollideType.Solid ? info.MaxBB[block].Y : 0;
					spawn.Y = y + height + Entity.Adjustment;
					return;
				}
			}
		}
	}
}