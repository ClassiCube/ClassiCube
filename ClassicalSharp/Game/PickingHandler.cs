// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using ClassicalSharp.Physics;
using OpenTK;
using OpenTK.Input;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp {

	public sealed class PickingHandler {
		
		Game game;
		InputHandler input;
		public PickingHandler(Game game, InputHandler input) {
			this.game = game;
			this.input = input;
		}

		internal DateTime lastClick = DateTime.MinValue;
		public void PickBlocks(bool cooldown, bool left, bool middle, bool right) {
			DateTime now = DateTime.UtcNow;
			double delta = (now - lastClick).TotalMilliseconds;
			if (cooldown && delta < 250) return; // 4 times per second
			
			lastClick = now;
			Inventory inv = game.Inventory;
			
			if (game.Server.UsingPlayerClick && !game.Gui.ActiveScreen.HandlesAllInput) {
				input.pickingId = -1;
				input.ButtonStateChanged(MouseButton.Left, left);
				input.ButtonStateChanged(MouseButton.Right, right);
				input.ButtonStateChanged(MouseButton.Middle, middle);
			}
			
			int btns = (left ? 1 : 0) + (right ? 1 : 0) + (middle ? 1 : 0);
			if (btns > 1 || game.Gui.ActiveScreen.HandlesAllInput || inv.Selected == Block.Invalid) return;
			
			// always play delete animations, even if we aren't picking a block.
			if (left) {
				game.HeldBlockRenderer.anim.SetClickAnim(true);
				byte id = game.Entities.GetClosetPlayer(game.LocalPlayer);
				if (id != EntityList.SelfID && game.Mode.PickEntity(id)) return;
			}
			if (!game.SelectedPos.Valid) return;
			
			if (middle) {
				Vector3I pos = game.SelectedPos.BlockPos;
				if (!game.World.IsValidPos(pos)) return;
				
				BlockID old = game.World.GetBlock(pos);
				game.Mode.PickMiddle(old);
			} else if (left) {
				Vector3I pos = game.SelectedPos.BlockPos;
				if (!game.World.IsValidPos(pos)) return;
				
				BlockID old = game.World.GetBlock(pos);
				if (game.BlockInfo.Draw[old] == DrawType.Gas || !inv.CanDelete[old]) return;
				game.Mode.PickLeft(old);
			} else if (right) {
				Vector3I pos = game.SelectedPos.TranslatedPos;
				if (!game.World.IsValidPos(pos)) return;
				
				BlockID old = game.World.GetBlock(pos);
				BlockID block = inv.Selected;
				if (game.AutoRotate)
					block = AutoRotate.RotateBlock(game, block);
				
				if (game.CanPick(old) || !inv.CanPlace[block]) return;
				if (!PickingHandler.CheckIsFree(game, block)) return;
				game.Mode.PickRight(old, block);
			}
		}
		
		public static bool CheckIsFree(Game game, BlockID block) {
			Vector3 pos = (Vector3)game.SelectedPos.TranslatedPos;
			BlockInfo info = game.BlockInfo;
			LocalPlayer p = game.LocalPlayer;
			
			if (info.Collide[block] != CollideType.Solid) return true;
			if (IntersectsOtherPlayers(game, pos, block)) return false;
			
			AABB blockBB = new AABB(pos + info.MinBB[block], pos + info.MaxBB[block]);
			// NOTE: We need to also test against nextPos here, because otherwise
			// we can fall through the block as collision is performed against nextPos
			AABB localBB = AABB.Make(p.Position, p.Size);
			localBB.Min.Y = Math.Min(p.interp.next.Pos.Y, localBB.Min.Y);
			
			if (p.Hacks.Noclip || !localBB.Intersects(blockBB)) return true;
			if (p.Hacks.CanPushbackBlocks && p.Hacks.PushbackPlacing && p.Hacks.Enabled)
				return PushbackPlace(game, blockBB);
			
			localBB.Min.Y += 0.25f + Entity.Adjustment;
			if (localBB.Intersects(blockBB)) return false;
			
			// Push player up if they are jumping and trying to place a block underneath them.
			Vector3 next = game.LocalPlayer.interp.next.Pos;
			next.Y = pos.Y + game.BlockInfo.MaxBB[block].Y + Entity.Adjustment;
			LocationUpdate update = LocationUpdate.MakePos(next, false);
			game.LocalPlayer.SetLocation(update, false);
			return true;
		}
		
		static bool PushbackPlace(Game game, AABB blockBB) {
			Vector3 newP = game.LocalPlayer.Position;
			Vector3 oldP = game.LocalPlayer.Position;
			
			// Offset position by the closest face
			PickedPos selected = game.SelectedPos;
			if (selected.Face == BlockFace.XMax) {
				newP.X = blockBB.Max.X + 0.5f;
			} else if (selected.Face == BlockFace.ZMax) {
				newP.Z = blockBB.Max.Z + 0.5f;
			} else if (selected.Face == BlockFace.XMin) {
				newP.X = blockBB.Min.X - 0.5f;
			} else if (selected.Face == BlockFace.ZMin) {
				newP.Z = blockBB.Min.Z - 0.5f;
			} else if (selected.Face == BlockFace.YMax) {
				newP.Y = blockBB.Min.Y + 1 + Entity.Adjustment;
			} else if (selected.Face == BlockFace.YMin) {
				newP.Y = blockBB.Min.Y - game.LocalPlayer.Size.Y - Entity.Adjustment;
			}
			
			Vector3I newLoc = Vector3I.Floor(newP);
			bool validPos = newLoc.X >= 0 && newLoc.Y >= 0 && newLoc.Z >= 0 &&
				newLoc.X < game.World.Width && newP.Z < game.World.Length;
			if (!validPos) return false;
			
			game.LocalPlayer.Position = newP;
			if (!game.LocalPlayer.Hacks.Noclip
			    && game.LocalPlayer.TouchesAny(b => game.BlockInfo.Collide[b] == CollideType.Solid)) {
				game.LocalPlayer.Position = oldP;
				return false;
			}
			
			game.LocalPlayer.Position = oldP;
			LocationUpdate update = LocationUpdate.MakePos(newP, false);
			game.LocalPlayer.SetLocation(update, false);
			return true;
		}
		
		static bool IntersectsOtherPlayers(Game game, Vector3 pos, BlockID block) {
			AABB blockBB = new AABB(pos + game.BlockInfo.MinBB[block],
			                        pos + game.BlockInfo.MaxBB[block]);
			
			for (int id = 0; id < EntityList.SelfID; id++) {
				Entity entity = game.Entities[id];
				if (entity == null) continue;
				
				AABB bounds = entity.Bounds;
				bounds.Min.Y += 1/32f; // when player is exactly standing on top of ground
				if (bounds.Intersects(blockBB)) return true;
			}
			return false;
		}
	}
}