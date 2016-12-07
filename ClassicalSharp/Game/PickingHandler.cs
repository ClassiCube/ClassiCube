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
				byte targetId = game.Entities.GetClosetPlayer(game.LocalPlayer);
				input.ButtonStateChanged(MouseButton.Left, left, targetId);
				input.ButtonStateChanged(MouseButton.Right, right, targetId);
				input.ButtonStateChanged(MouseButton.Middle, middle, targetId);
			}
			
			int buttonsDown = (left ? 1 : 0) + (right ? 1 : 0) + (middle ? 1 : 0);
			if (buttonsDown > 1 || game.Gui.ActiveScreen.HandlesAllInput ||
			   inv.HeldBlock == Block.Air) return;
			
			// always play delete animations, even if we aren't picking a block.
			if (left) game.HeldBlockRenderer.anim.SetClickAnim(true);
			if (!game.SelectedPos.Valid) return;
			BlockInfo info = game.BlockInfo;
			
			if (middle) {
				Vector3I pos = game.SelectedPos.BlockPos;
				if (!game.World.IsValidPos(pos)) return;
				byte old = game.World.GetBlock(pos);
				
				if (info.Draw[old] != DrawType.Gas && (inv.CanPlace[old] || inv.CanDelete[old])) {
					for (int i = 0; i < inv.Hotbar.Length; i++) {
						if (inv.Hotbar[i] == old) {
							inv.HeldBlockIndex = i; return;
						}
					}
					inv.HeldBlock = old;
				}
			} else if (left) {
				Vector3I pos = game.SelectedPos.BlockPos;
				if (!game.World.IsValidPos(pos)) return;
				byte old = game.World.GetBlock(pos);
				
				if (info.Draw[old] != DrawType.Gas && inv.CanDelete[old]) {
					game.UpdateBlock(pos.X, pos.Y, pos.Z, 0);
					game.UserEvents.RaiseBlockChanged(pos, old, 0);
				}
			} else if (right) {
				//[1:28:03 PM]  <UnknownShadow200+> well goodlyay you have game.SelectedPos.Intersect
				//[1:28:17 PM]  <UnknownShadow200+> that's the exact floating point coordinates that the mouse clicked on the block
				//[1:31:45 PM]  <UnknownShadow200+> e.g. the block clicked on might be (32, 34, 63)
				//[1:31:55 PM]  <UnknownShadow200+> Intersect might be (33, 34.51049, 63.44751)
				Vector3I pos = game.SelectedPos.TranslatedPos;
				Vector3 posExact = game.SelectedPos.Intersect;
				if (!game.World.IsValidPos(pos)) return;
				byte old = game.World.GetBlock(pos);
				byte block = (byte)inv.HeldBlock;
				if (game.autoRotate) {
					string[] blockNameSplit = info.Name[block].ToUpper().Split('-');
					bool isDirectional = false;
					bool isHeight = false;
					bool isCorner = false;
					
					if (blockNameSplit.Length == 2) {
						switch (blockNameSplit[1]) {
							case "N": case "E": case "W": case "S":
							isDirectional = true;
							break;
						}
						switch (blockNameSplit[1]) {
							case "U": case "D":
							isHeight = true;
							break;
						}
						switch (blockNameSplit[1]) {
							//NW NE
							//SW SE
							case "NW": case "NE": case "SW": case "SE":
							isCorner = true;
							break;
						}
					}
					if (isDirectional) {
						Vector3 southEast = new Vector3 (1,0,1);
						Vector3 southWest = new Vector3 (-1, 0, 1);
						Vector3 posExactFlat = posExact; posExactFlat.Y = 0;
						Vector3 southEastToPoint = posExactFlat - new Vector3 (pos.X, 0, pos.Z);
						Vector3 southWestToPoint = posExactFlat - new Vector3 (pos.X +1, 0, pos.Z);
						float dotSouthEast = Vector3.Dot(southEastToPoint, southWest);
						float dotSouthWest= Vector3.Dot(southWestToPoint, southEast);
						string direction;
						if (dotSouthEast <= 0) { // NorthEast
							if (dotSouthWest <= 0) { //NorthWest
								//North
								direction = "N";
							} else { //SouthEast
								//East
								direction = "E";
							}
						} else { //SouthWest
							if (dotSouthWest <= 0) { //NorthWest
								//West
								direction = "W";
							} else { //SouthEast
								//South	
								direction = "S";
							}
						}
						
						if (direction != blockNameSplit[1]) {
							string newBlockName = blockNameSplit[0] + "-" + direction;
							int newBlockID = game.BlockInfo.FindID(newBlockName);
							if (newBlockID != -1) {
								block = (byte)newBlockID;
								//game.Chat.Add("Substituted " + block + " for " + newBlockID + ".");
							} else {
								//game.Chat.Add("could not find " + newBlockName + ".");
							}
						}
					}
					if (isHeight) {
						string height = "D";
						float pickedHeight = posExact.Y - pos.Y;
						//game.Chat.Add("pickedHeight: " + pickedHeight + ".");
						
						if (pickedHeight >= 0.5f) {
							height = "U";
						}
						
						if (height != blockNameSplit[1]) {
							string newBlockName = blockNameSplit[0] + "-" + height;
							int newBlockID = game.BlockInfo.FindID(newBlockName);
							if (newBlockID != -1) {
								block = (byte)newBlockID;
								//game.Chat.Add("Substituted " + block + " for " + newBlockID + ".");
							} else {
								//game.Chat.Add("could not find " + newBlockName + ".");
							}
						}
					}
					
					if (isCorner) {
						string corner = "NW";
						Vector3 pickedCorner = posExact - (Vector3)pos;
						//game.Chat.Add("pickedHeight: " + pickedHeight + ".");
						
						//NW NE
						//SW SE
						if (pickedCorner.X < 0.5f && pickedCorner.Z < 0.5f) {
							corner = "NW";
						}
						if (pickedCorner.X >= 0.5f && pickedCorner.Z < 0.5f) {
							corner = "NE";
						}
						if (pickedCorner.X < 0.5f && pickedCorner.Z >= 0.5f) {
							corner = "SW";
						}
						if (pickedCorner.X >= 0.5f && pickedCorner.Z >= 0.5f) {
							corner = "SE";
						}
						
						if (corner != blockNameSplit[1]) {
							string newBlockName = blockNameSplit[0] + "-" + corner;
							int newBlockID = game.BlockInfo.FindID(newBlockName);
							if (newBlockID != -1) {
								block = (byte)newBlockID;
								//game.Chat.Add("Substituted " + block + " for " + newBlockID + ".");
							} else {
								//game.Chat.Add("could not find " + newBlockName + ".");
							}
						}
					}
					
				}
				
				
				if (!game.CanPick(old) && inv.CanPlace[block] && CheckIsFree(game.SelectedPos, block)) {
					game.UpdateBlock(pos.X, pos.Y, pos.Z, block);
					game.UserEvents.RaiseBlockChanged(pos, old, block);
				}
			}
		}
		
		bool CheckIsFree(PickedPos selected, byte block) {
			Vector3 pos = (Vector3)selected.TranslatedPos;
			BlockInfo info = game.BlockInfo;
			LocalPlayer p = game.LocalPlayer;
			
			if (info.Collide[block] != CollideType.Solid) return true;
			if (IntersectsOtherPlayers(pos, block)) return false;
			
			AABB blockBB = new AABB(pos + info.MinBB[block], pos + info.MaxBB[block]);
			// NOTE: We need to also test against nextPos here, because otherwise
			// we can fall through the block as collision is performed against nextPos
			AABB localBB = AABB.Make(p.Position, p.Size);
			localBB.Min.Y = Math.Min(p.nextPos.Y, localBB.Min.Y);
			
			if (p.Hacks.Noclip || !localBB.Intersects(blockBB)) return true;
			if (p.Hacks.CanPushbackBlocks && p.Hacks.PushbackPlacing && p.Hacks.Enabled)
				return PushbackPlace(selected, blockBB);
			
			localBB.Min.Y += 0.25f + Entity.Adjustment;
			if (localBB.Intersects(blockBB)) return false;
			
			// Push player up if they are jumping and trying to place a block underneath them.
			Vector3 next = game.LocalPlayer.nextPos;
			next.Y = pos.Y + game.BlockInfo.MaxBB[block].Y + Entity.Adjustment;
			LocationUpdate update = LocationUpdate.MakePos(next, false);
			game.LocalPlayer.SetLocation(update, false);
			return true;
		}
		
		bool PushbackPlace(PickedPos selected, AABB blockBB) {
			Vector3 newP = game.LocalPlayer.Position;
			Vector3 oldP = game.LocalPlayer.Position;
			
			// Offset position by the closest face
			if (selected.BlockFace == BlockFace.XMax) {
				newP.X = blockBB.Max.X + 0.5f;
			} else if (selected.BlockFace == BlockFace.ZMax) {
				newP.Z = blockBB.Max.Z + 0.5f;
			} else if (selected.BlockFace == BlockFace.XMin) {
				newP.X = blockBB.Min.X - 0.5f;
			} else if (selected.BlockFace == BlockFace.ZMin) {
				newP.Z = blockBB.Min.Z - 0.5f;
			} else if (selected.BlockFace == BlockFace.YMax) {
				newP.Y = blockBB.Min.Y + 1 + Entity.Adjustment;
			} else if (selected.BlockFace == BlockFace.YMin) {
				newP.Y = blockBB.Min.Y - game.LocalPlayer.Size.Y - Entity.Adjustment;
			}
			
			Vector3I newLoc = Vector3I.Floor(newP);
			bool validPos = newLoc.X >= 0 && newLoc.Y >= 0 && newLoc.Z >= 0 &&
				newLoc.X < game.World.Width && newP.Z < game.World.Length;
			if (!validPos) return false;
			
			game.LocalPlayer.Position = newP;
			if (!game.LocalPlayer.Hacks.Noclip
			   && game.LocalPlayer.TouchesAny(CannotPassThrough)) {
				game.LocalPlayer.Position = oldP;
				return false;
			}
			
			game.LocalPlayer.Position = oldP;
			LocationUpdate update = LocationUpdate.MakePos(newP, false);
			game.LocalPlayer.SetLocation(update, false);
			return true;
		}
		
		bool CannotPassThrough(byte block) {
			return game.BlockInfo.Collide[block] == CollideType.Solid;
		}
		
		bool IntersectsOtherPlayers(Vector3 pos, byte newType) {
			AABB blockBB = new AABB(pos + game.BlockInfo.MinBB[newType],
			                                      pos + game.BlockInfo.MaxBB[newType]);
			
			for (int id = 0; id < 255; id++) {
				Player player = game.Entities[id];
				if (player == null) continue;
				AABB bounds = player.Bounds;
				bounds.Min.Y += 1/32f; // when player is exactly standing on top of ground
				if (bounds.Intersects(blockBB)) return true;
			}
			return false;
		}
	}
}