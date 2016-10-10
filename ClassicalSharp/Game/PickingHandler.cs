// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Physics;
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
			
			if( game.Server.UsingPlayerClick && !game.Gui.ActiveScreen.HandlesAllInput ) {
				byte targetId = game.Entities.GetClosetPlayer( game.LocalPlayer );
				input.ButtonStateChanged( MouseButton.Left, left, targetId );
				input.ButtonStateChanged( MouseButton.Right, right, targetId );
				input.ButtonStateChanged( MouseButton.Middle, middle, targetId );
			}
			
			int buttonsDown = (left ? 1 : 0) + (right ? 1 : 0) + (middle ? 1 : 0);
			if( buttonsDown > 1 || game.Gui.ActiveScreen.HandlesAllInput ||
			   inv.HeldBlock == Block.Air ) return;
			
			// always play delete animations, even if we aren't picking a block.
			if( left ) game.HeldBlockRenderer.anim.SetClickAnim( true );
			if( !game.SelectedPos.Valid ) return;
			BlockInfo info = game.BlockInfo;
			
			if( middle ) {
				Vector3I pos = game.SelectedPos.BlockPos;
				if( !game.World.IsValidPos( pos ) ) return;
				byte old = game.World.GetBlock( pos );
				
				if( !info.IsAir[old] && (inv.CanPlace[old] || inv.CanDelete[old]) ) {
					for( int i = 0; i < inv.Hotbar.Length; i++ ) {
						if( inv.Hotbar[i] == old ) {
							inv.HeldBlockIndex = i; return;
						}
					}
					inv.HeldBlock = old;
				}
			} else if( left ) {
				Vector3I pos = game.SelectedPos.BlockPos;
				if( !game.World.IsValidPos( pos ) ) return;
				byte old = game.World.GetBlock( pos );
				
				if( !info.IsAir[old] && inv.CanDelete[old] ) {
					game.UpdateBlock( pos.X, pos.Y, pos.Z, 0 );
					game.UserEvents.RaiseBlockChanged( pos, old, 0 );
				}
			} else if( right ) {
				Vector3I pos = game.SelectedPos.TranslatedPos;
				if( !game.World.IsValidPos( pos ) ) return;
				byte old = game.World.GetBlock( pos );
				byte block = (byte)inv.HeldBlock;
				
				if( !game.CanPick( old ) && inv.CanPlace[block] && CheckIsFree( game.SelectedPos, block ) ) {
					game.UpdateBlock( pos.X, pos.Y, pos.Z, block );
					game.UserEvents.RaiseBlockChanged( pos, old, block );
				}
			}
		}
		
		bool CheckIsFree( PickedPos selected, byte block ) {
			Vector3 pos = (Vector3)selected.TranslatedPos;
			BlockInfo info = game.BlockInfo;
			LocalPlayer p = game.LocalPlayer;
			
			if( info.Collide[block] != CollideType.Solid ) return true;
			if( IntersectsOtherPlayers( pos, block ) ) return false;
			
			AABB blockBB = new AABB( pos + info.MinBB[block], pos + info.MaxBB[block] );
			// NOTE: We need to also test against nextPos here, because otherwise
			// we can fall through the block as collision is performed against nextPos
			AABB localBB = AABB.Make( p.Position, p.Size );
			localBB.Min.Y = Math.Min( p.nextPos.Y, localBB.Min.Y );
			
			if( p.Hacks.Noclip || !localBB.Intersects( blockBB ) ) return true;
			if( p.Hacks.CanPushbackBlocks && p.Hacks.PushbackPlacing && p.Hacks.Enabled )
				return PushbackPlace( selected, blockBB );
			
			localBB.Min.Y += 0.25f + Entity.Adjustment;
			if( localBB.Intersects( blockBB ) ) return false;
			
			// Push player up if they are jumping and trying to place a block underneath them.
			Vector3 next = game.LocalPlayer.nextPos;
			next.Y = pos.Y + game.BlockInfo.MaxBB[block].Y + Entity.Adjustment;
			LocationUpdate update = LocationUpdate.MakePos( next, false );
			game.LocalPlayer.SetLocation( update, false );
			return true;
		}
		
		bool PushbackPlace( PickedPos selected, AABB blockBB ) {
			Vector3 newP = game.LocalPlayer.Position;
			Vector3 oldP = game.LocalPlayer.Position;
			
			// Offset position by the closest face
			if( selected.BlockFace == BlockFace.XMax ) {
				newP.X = blockBB.Max.X + 0.5f;
			} else if( selected.BlockFace == BlockFace.ZMax ) {
				newP.Z = blockBB.Max.Z + 0.5f;
			} else if( selected.BlockFace == BlockFace.XMin ) {
				newP.X = blockBB.Min.X - 0.5f;
			} else if( selected.BlockFace == BlockFace.ZMin ) {
				newP.Z = blockBB.Min.Z - 0.5f;
			} else if( selected.BlockFace == BlockFace.YMax ) {
				newP.Y = blockBB.Min.Y + 1 + Entity.Adjustment;
			} else if( selected.BlockFace == BlockFace.YMin ) {
				newP.Y = blockBB.Min.Y - game.LocalPlayer.Size.Y - Entity.Adjustment;
			}
			
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
			AABB blockBB = new AABB( pos + game.BlockInfo.MinBB[newType],
			                                      pos + game.BlockInfo.MaxBB[newType] );
			
			for( int id = 0; id < 255; id++ ) {
				Player player = game.Entities[id];
				if( player == null ) continue;
				AABB bounds = player.Bounds;
				bounds.Min.Y += 1/32f; // when player is exactly standing on top of ground
				if( bounds.Intersects( blockBB ) ) return true;
			}
			return false;
		}
	}
}