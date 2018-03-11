// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using ClassicalSharp.Entities;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp {
	
	/// <summary> Performs automatic rotation of directional blocks. </summary>
	public static class AutoRotate {
		
		public static BlockID RotateBlock(Game game, BlockID block) {
			string name = BlockInfo.Name[block];
			int dirIndex = name.LastIndexOf('-');
			if (dirIndex == -1) return block; // not a directional block
			
			string dir = name.Substring(dirIndex + 1);
			name = name.Substring(0, dirIndex);
			Vector3 offset = game.SelectedPos.Intersect - (Vector3)game.SelectedPos.TranslatedPos;
			
			if (Utils.CaselessEquals(dir, "nw") || Utils.CaselessEquals(dir, "ne") ||
			    Utils.CaselessEquals(dir, "sw") || Utils.CaselessEquals(dir, "se")) {
				return RotateCorner(game, block, name, offset);
			} else if (Utils.CaselessEquals(dir, "u") || Utils.CaselessEquals(dir, "d")) {
				return RotateVertical(game, block, name, offset);
			} else if (Utils.CaselessEquals(dir, "n") || Utils.CaselessEquals(dir, "w") ||
			           Utils.CaselessEquals(dir, "s") || Utils.CaselessEquals(dir, "e")) {
				return RotateDirection(game, block, name, offset);
			} else if (Utils.CaselessEquals(dir, "UD") || Utils.CaselessEquals(dir, "WE") ||
			           Utils.CaselessEquals(dir, "NS")) {
				return RotateOther(game, block, name, offset);
			}
			return block;
		}
		
		static BlockID RotateCorner(Game game, BlockID block, string name, Vector3 offset) {
			if (offset.X < 0.5f && offset.Z < 0.5f) {
				return Find(game, block, name + "-NW");
			} else if (offset.X >= 0.5f && offset.Z < 0.5f) {
				return Find(game, block, name + "-NE");
			} else if (offset.X < 0.5f && offset.Z >= 0.5f) {
				return Find(game, block, name + "-SW");
			} else if (offset.X >= 0.5f && offset.Z >= 0.5f) {
				return Find(game, block, name + "-SE");
			}
			return block;
		}

		static BlockID RotateVertical(Game game, BlockID block, string name, Vector3 offset) {
			string height = offset.Y >= 0.5f ? "-U" : "-D";
			return Find(game, block, name + height);
		}
		
		static BlockID RotateOther(Game game, BlockID block, string name, Vector3 offset) {
			// Fence type blocks
			if (BlockInfo.FindID(name + "-UD") == -1) {
				float headY = LocationUpdate.Clamp(game.LocalPlayer.HeadY);
				if (headY < 45 || (headY >= 135 && headY < 225) || headY > 315)
					return Find(game, block, name + "-WE");
				return Find(game, block, name + "-NS");
			}
			
			// Thin pillar type blocks
			BlockFace face = game.SelectedPos.Face;
			if (face == BlockFace.YMax || face == BlockFace.YMin)
				return Find(game, block, name + "-UD");
			if (face == BlockFace.XMax || face == BlockFace.XMin)
				return Find(game, block, name + "-WE");
			if (face == BlockFace.ZMax || face == BlockFace.ZMin)
				return Find(game, block, name + "-NS");
			return block;
		}
		
		static BlockID RotateDirection(Game game, BlockID block, string name, Vector3 offset) {
			float headY = LocationUpdate.Clamp(game.LocalPlayer.HeadY);
			if (headY >= 45  && headY < 135)
				return Find(game, block, name + "-E");
			if (headY >= 135 && headY < 225)
				return Find(game, block, name + "-S");
			if (headY >= 225 && headY < 315)
				return Find(game, block, name + "-W");
			return Find(game, block, name + "-N");
		}
		
		static BlockID Find(Game game, BlockID block, string name) {
			int rotated = BlockInfo.FindID(name);
			if (rotated != -1) return (BlockID)rotated;
			return block;
		}
	}
}