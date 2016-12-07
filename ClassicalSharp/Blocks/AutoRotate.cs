// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using System.Collections.Generic;
using ClassicalSharp.Blocks;
using OpenTK;

namespace ClassicalSharp {
	
	/// <summary> Performs automatic rotation of directional blocks. </summary>
	public static class AutoRotate {
		
		public static byte RotateBlock(Game game, byte block) {
			string name = game.BlockInfo.Name[block];
			int sepIndex = name.LastIndexOf('-');
			if (sepIndex == -1) return block; // not a directional block
			
			string sep = name.Substring(sepIndex + 1);
			name = name.Substring(0, sepIndex);
			Vector3 offset = game.SelectedPos.Intersect - (Vector3)game.SelectedPos.TranslatedPos;
			
			if (Utils.CaselessEquals(sep, "nw") || Utils.CaselessEquals(sep, "ne") ||
			    Utils.CaselessEquals(sep, "sw") || Utils.CaselessEquals(sep, "se")) {
				return RotateCorner(game, block, name, offset);
			} else if (Utils.CaselessEquals(sep, "u") || Utils.CaselessEquals(sep, "d")) {
				return RotateVertical(game, block, name, offset);
			} else if (Utils.CaselessEquals(sep, "n") || Utils.CaselessEquals(sep, "w") ||
			           Utils.CaselessEquals(sep, "s") || Utils.CaselessEquals(sep, "e")) {
				return RotateDirection(game, block, name, offset);
			} else if (Utils.CaselessEquals(sep, "UD") || Utils.CaselessEquals(sep, "WE") ||
			           Utils.CaselessEquals(sep, "NS")) {
				return RotatePillar(game, block, name, offset);
			}
			return block;
		}
		
		static byte RotateCorner(Game game, byte block, string name, Vector3 offset) {
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

		static byte RotateVertical(Game game, byte block, string name, Vector3 offset) {
			string height = offset.Y >= 0.5f ? "-U" : "-D";
			return Find(game, block, name + height);
		}
		
		static byte RotatePillar(Game game, byte block, string name, Vector3 offset) {
			BlockFace selectedFace = game.SelectedPos.BlockFace;
			string face = "-UD";
			if (selectedFace == BlockFace.YMax || selectedFace == BlockFace.YMin) { face = "-UD"; }
			else if (selectedFace == BlockFace.XMax || selectedFace == BlockFace.XMin) { face = "-WE"; }
			else if (selectedFace == BlockFace.ZMax || selectedFace == BlockFace.ZMin) { face = "-NS"; }
			return Find(game, block, name + face);
		}
		
		static byte RotateDirection(Game game, byte block, string name, Vector3 offset) {
			Vector3 southEast = new Vector3 (1,0,1);
			Vector3 southWest = new Vector3 (-1, 0, 1);
			
			Vector3I pos = game.SelectedPos.TranslatedPos;
			Vector3 posExact = game.SelectedPos.Intersect;
			Vector3 posExactFlat = posExact; posExactFlat.Y = 0;
			Vector3 southEastToPoint = posExactFlat - new Vector3 (pos.X, 0, pos.Z);
			Vector3 southWestToPoint = posExactFlat - new Vector3 (pos.X +1, 0, pos.Z);
			
			float dotSouthEast = Vector3.Dot(southEastToPoint, southWest);
			float dotSouthWest= Vector3.Dot(southWestToPoint, southEast);
			if (dotSouthEast <= 0) { // NorthEast
				if (dotSouthWest <= 0) { //NorthWest
					return Find(game, block, name + "-N");
				} else { //SouthEast
					return Find(game, block, name + "-E");
				}
			} else { //SouthWest
				if (dotSouthWest <= 0) { //NorthWest
					return Find(game, block, name + "-W");
				} else { //SouthEast
					return Find(game, block, name + "-S");
				}
			}
		}
		
		static byte Find(Game game, byte block, string name) {
			int rotated = game.BlockInfo.FindID(name);
			if (rotated != -1) return (byte)rotated;
			return block;
		}
	}
}