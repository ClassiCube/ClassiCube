// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using OpenTK;
using OpenTK.Input;

namespace ClassicalSharp {

	public sealed class PickingHandler {
		
		Game game;
		InputHandler input;
		public PickingHandler( Game game, InputHandler input ) {
			this.game = game;
			this.input = input;
		}		

		internal DateTime lastClick = DateTime.MinValue;
		public void PickBlocks( bool cooldown, bool left, bool middle, bool right ) {
			DateTime now = DateTime.UtcNow;
			double delta = (now - lastClick).TotalMilliseconds;
			if( cooldown && delta < 250 ) return; // 4 times per second
			
			lastClick = now;
			Inventory inv = game.Inventory;
			
			if( game.Network.UsingPlayerClick && !game.ScreenLockedInput ) {
				byte targetId = game.Players.GetClosetPlayer( game.LocalPlayer );
				input.ButtonStateChanged( MouseButton.Left, left, targetId );
				input.ButtonStateChanged( MouseButton.Right, right, targetId );
				input.ButtonStateChanged( MouseButton.Middle, middle, targetId );
			}
			
			int buttonsDown = (left ? 1 : 0) + (right ? 1 : 0) + (middle ? 1 : 0);
			if( buttonsDown > 1 || game.ScreenLockedInput || inv.HeldBlock == Block.Air ) return;
			
			// always play delete animations, even if we aren't picking a block.
			if( left ) game.BlockHandRenderer.SetAnimationClick( true );
			if( !game.SelectedPos.Valid ) return;
			
			if( middle ) {
				Vector3I pos = game.SelectedPos.BlockPos;
				byte block = 0;
				if( game.World.IsValidPos( pos ) && (block = game.World.GetBlock( pos )) != 0
				   && (inv.CanPlace[block] || inv.CanDelete[block]) ) {
					
					for( int i = 0; i < inv.Hotbar.Length; i++ ) {
						if( inv.Hotbar[i] == (Block)block ) {
							inv.HeldBlockIndex = i; return;
						}
					}
					inv.HeldBlock = (Block)block;
				}
			} else if( left ) {
				Vector3I pos = game.SelectedPos.BlockPos;
				byte block = 0;
				if( game.World.IsValidPos( pos ) && (block = game.World.GetBlock( pos )) != 0
				   && inv.CanDelete[block] ) {
					game.ParticleManager.BreakBlockEffect( pos, block );
					game.AudioPlayer.PlayDigSound( game.BlockInfo.DigSounds[block] );
					game.UpdateBlock( pos.X, pos.Y, pos.Z, 0 );
					game.Network.SendSetBlock( pos.X, pos.Y, pos.Z, false, (byte)inv.HeldBlock );
				}
			} else if( right ) {
				Vector3I pos = game.SelectedPos.TranslatedPos;
				if( !game.World.IsValidPos( pos ) ) return;
				
				byte block = (byte)inv.HeldBlock;
				if( !game.CanPick( game.World.GetBlock( pos ) ) && inv.CanPlace[block]
				   && CheckIsFree( game.SelectedPos, block ) ) {
					game.UpdateBlock( pos.X, pos.Y, pos.Z, block );
					game.AudioPlayer.PlayDigSound( game.BlockInfo.StepSounds[block] );
					game.Network.SendSetBlock( pos.X, pos.Y, pos.Z, true, block );
					game.BlockHandRenderer.SetAnimationClick( false );
				}
			}
		}
		
		bool CheckIsFree( PickedPos selected, byte newBlock ) {
			Vector3 pos = (Vector3)selected.TranslatedPos;
			if( !CannotPassThrough( newBlock ) ) return true;
			if( IntersectsOtherPlayers( pos, newBlock ) ) return false;
			
			BoundingBox blockBB = new BoundingBox( pos + game.BlockInfo.MinBB[newBlock],
			                                      pos + game.BlockInfo.MaxBB[newBlock] );
			BoundingBox localBB = game.LocalPlayer.CollisionBounds;
			
			if( game.LocalPlayer.Hacks.Noclip || !localBB.Intersects( blockBB ) ) return true;
			HacksComponent hacks = game.LocalPlayer.Hacks;
			if( hacks.CanPushbackBlocks && hacks.PushbackPlacing && hacks.Enabled )
				return PushbackPlace( selected, blockBB );
			
			localBB.Min.Y += 0.25f + Entity.Adjustment;
			if( localBB.Intersects( blockBB ) ) return false;
			
			// Push player up if they are jumping and trying to place a block underneath them.
			Vector3 p = game.LocalPlayer.Position;
			p.Y = pos.Y + game.BlockInfo.MaxBB[newBlock].Y + Entity.Adjustment;
			LocationUpdate update = LocationUpdate.MakePos( p, false );
			game.LocalPlayer.SetLocation( update, false );
			return true;
		}
		
		bool PushbackPlace( PickedPos selected, BoundingBox blockBB ) {
			Vector3 newP = game.LocalPlayer.Position;
			Vector3 oldP = game.LocalPlayer.Position;
			
			// Offset position by the closest face
			if( selected.BlockFace == CpeBlockFace.XMax )
				newP.X = blockBB.Max.X + 0.5f;
			else if( selected.BlockFace == CpeBlockFace.ZMax )
				newP.Z = blockBB.Max.Z + 0.5f;
			else if( selected.BlockFace == CpeBlockFace.XMin )
				newP.X = blockBB.Min.X - 0.5f;
			else if( selected.BlockFace == CpeBlockFace.ZMin )
				newP.Z = blockBB.Min.Z - 0.5f;
			else if( selected.BlockFace == CpeBlockFace.YMax )
				newP.Y = blockBB.Min.Y + 1 + Entity.Adjustment;
			else if( selected.BlockFace == CpeBlockFace.YMin )
				newP.Y = blockBB.Min.Y - game.LocalPlayer.CollisionSize.Y - Entity.Adjustment;
			
			Vector3I newLoc = Vector3I.Floor( newP );
			bool validPos = newLoc.X >= 0 && newLoc.Y >= 0 && newLoc.Z >= 0 &&
				newLoc.X < game.World.Width && newP.Z < game.World.Length;
			if( !validPos ) return false;
			
			game.LocalPlayer.Position = newP;
			if( !game.LocalPlayer.Hacks.Noclip
			   && game.LocalPlayer.TouchesAny( CannotPassThrough ) ) {
				game.LocalPlayer.Position = oldP;
				return false;
			}
			
			game.LocalPlayer.Position = oldP;
			LocationUpdate update = LocationUpdate.MakePos( newP, false );
			game.LocalPlayer.SetLocation( update, false );
			return true;
		}
		
		bool CannotPassThrough( byte block ) {
			return game.BlockInfo.Collide[block] == CollideType.Solid;
		}
		
		bool IntersectsOtherPlayers( Vector3 pos, byte newType ) {
			BoundingBox blockBB = new BoundingBox( pos + game.BlockInfo.MinBB[newType],
			                                      pos + game.BlockInfo.MaxBB[newType] );
			
			for( int id = 0; id < 255; id++ ) {
				Player player = game.Players[id];
				if( player == null ) continue;
				BoundingBox bounds = player.CollisionBounds;
				bounds.Min.Y += 1/32f; // when player is exactly standing on top of ground
				if( bounds.Intersects( blockBB ) ) return true;
			}
			return false;
		}
	}
}