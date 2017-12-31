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

			if (game.Gui.ActiveScreen.HandlesAllInput || !inv.CanPick) return;

			if (left) {
				if (game.Mode.PickingLeft()) return;
				Vector3I pos = game.SelectedPos.BlockPos;
				if (!game.SelectedPos.Valid || !game.World.IsValidPos(pos)) return;
				
				BlockID old = game.World.GetBlock(pos);
				if (BlockInfo.Draw[old] == DrawType.Gas || !BlockInfo.CanDelete[old]) return;
				game.Mode.PickLeft(old);
			} else if (right) {
				if (game.Mode.PickingRight()) return;
				Vector3I pos = game.SelectedPos.TranslatedPos;
				if (!game.SelectedPos.Valid || !game.World.IsValidPos(pos)) return;
				
				BlockID old = game.World.GetBlock(pos);
				BlockID block = inv.Selected;
				if (game.AutoRotate)
					block = AutoRotate.RotateBlock(game, block);
				
				if (game.CanPick(old) || !BlockInfo.CanPlace[block]) return;
				if (!PickingHandler.CheckIsFree(game, block)) return;
				game.Mode.PickRight(old, block);
			} else if (middle) {
				Vector3I pos = game.SelectedPos.BlockPos;
				if (!game.SelectedPos.Valid || !game.World.IsValidPos(pos)) return;
				
				BlockID old = game.World.GetBlock(pos);
				game.Mode.PickMiddle(old);
			}
		}
		
		public static bool CheckIsFree(Game game, BlockID block) {
			Vector3 pos = (Vector3)game.SelectedPos.TranslatedPos;
			LocalPlayer p = game.LocalPlayer;
			
			if (BlockInfo.Collide[block] != CollideType.Solid) return true;
			if (IntersectsOtherPlayers(game, pos, block)) return false;
			
			AABB blockBB = new AABB(pos + BlockInfo.MinBB[block], pos + BlockInfo.MaxBB[block]);
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
			next.Y = pos.Y + BlockInfo.MaxBB[block].Y + Entity.Adjustment;
			LocationUpdate update = LocationUpdate.MakePos(next, false);
			game.LocalPlayer.SetLocation(update, false);
			return true;
		}
		
		static bool PushbackPlace(Game game, AABB blockBB) {
			LocalPlayer p = game.LocalPlayer;
			Vector3 curPos = p.Position, adjPos = p.Position;

			
			// Offset position by the closest face
			PickedPos selected = game.SelectedPos;
			if (selected.Face == BlockFace.XMax) {
				adjPos.X = blockBB.Max.X + 0.5f;
			} else if (selected.Face == BlockFace.ZMax) {
				adjPos.Z = blockBB.Max.Z + 0.5f;
			} else if (selected.Face == BlockFace.XMin) {
				adjPos.X = blockBB.Min.X - 0.5f;
			} else if (selected.Face == BlockFace.ZMin) {
				adjPos.Z = blockBB.Min.Z - 0.5f;
			} else if (selected.Face == BlockFace.YMax) {
				adjPos.Y = blockBB.Min.Y + 1 + Entity.Adjustment;
			} else if (selected.Face == BlockFace.YMin) {
				adjPos.Y = blockBB.Min.Y - p.Size.Y - Entity.Adjustment;
			}

			// exclude exact map boundaries, otherwise player can get stuck outside map
			bool validPos = 
				adjPos.X > 0 && adjPos.Y >= 0 && adjPos.Z > 0 &&
				adjPos.X < game.World.Width && adjPos.Z < game.World.Length;
			if (!validPos) return false;
			
			p.Position = adjPos;
			if (!p.Hacks.Noclip && p.TouchesAny(p.Bounds, touchesAnySolid)) {
				p.Position = curPos;
				return false;
			}
			
			p.Position = curPos;
			LocationUpdate update = LocationUpdate.MakePos(adjPos, false);
			p.SetLocation(update, false);
			return true;
		}
				
		static Predicate<BlockID> touchesAnySolid = IsSolidCollide;
		static bool IsSolidCollide(BlockID b) { return BlockInfo.Collide[b] == CollideType.Solid; }
		
		static bool IntersectsOtherPlayers(Game game, Vector3 pos, BlockID block) {
			AABB blockBB = new AABB(pos + BlockInfo.MinBB[block],
			                        pos + BlockInfo.MaxBB[block]);
			
			for (int id = 0; id < EntityList.SelfID; id++) {
				Entity entity = game.Entities.List[id];
				if (entity == null) continue;
				
				AABB bounds = entity.Bounds;
				bounds.Min.Y += 1/32f; // when player is exactly standing on top of ground
				if (bounds.Intersects(blockBB)) return true;
			}
			return false;
		}
	}
}