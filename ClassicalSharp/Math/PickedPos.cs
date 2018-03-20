// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;
using BlockID = System.UInt16;

namespace ClassicalSharp {

	/// <summary> Describes the picked/selected block by the user and its position. </summary>
	public class PickedPos {
		
		/// <summary> Minimum world coordinates of the block's bounding box. </summary>
		public Vector3 Min;
		
		/// <summary> Maximum world coordinates of the block's bounding box. </summary>
		public Vector3 Max;
		
		/// <summary> Exact world coordinates at which the ray intersected this block. </summary>
		public Vector3 Intersect;
		
		/// <summary> Integer world coordinates of the block. </summary>
		public Vector3I BlockPos;
		
		/// <summary> Integer world coordinates of the neighbouring block that is closest to the player. </summary>
		public Vector3I TranslatedPos;
		
		/// <summary> Whether this instance actually has a selected block currently. </summary>
		public bool Valid = true;
		
		/// <summary> Face of the picked block that is closet to the player. </summary>
		public BlockFace Face;
		
		/// <summary> Block ID of the picked block. </summary>
		public BlockID Block;
		
		/// <summary> Mark this as having a selected block, and
		/// calculates the closest face of the selected block's position. </summary>
		public void SetAsValid(int x, int y, int z, Vector3 min, Vector3 max,
		                       BlockID block, Vector3 intersect) {
			Vector3I pos = new Vector3I(x, y, z);
			Valid = true;
			BlockPos = pos;
			Block = block;
			Min = min; Max = max;
			Intersect = intersect;
			
			float dist = float.PositiveInfinity;
			TestAxis(intersect.X - Min.X, ref dist, BlockFace.XMin);
			TestAxis(intersect.X - Max.X, ref dist, BlockFace.XMax);
			TestAxis(intersect.Y - Min.Y, ref dist, BlockFace.YMin);
			TestAxis(intersect.Y - Max.Y, ref dist, BlockFace.YMax);
			TestAxis(intersect.Z - Min.Z, ref dist, BlockFace.ZMin);
			TestAxis(intersect.Z - Max.Z, ref dist, BlockFace.ZMax);
			
			switch (Face) {
				case BlockFace.XMin: pos.X--; break;
				case BlockFace.XMax: pos.X++; break;
				case BlockFace.YMin: pos.Y--; break;
				case BlockFace.YMax: pos.Y++; break;
				case BlockFace.ZMin: pos.Z--; break;
				case BlockFace.ZMax: pos.Z++; break;
			}
			TranslatedPos = pos;
		}
		
		/// <summary> Mark this as not having a selected block. </summary>
		public void SetAsInvalid() {
			Valid = false;
			BlockPos = Vector3I.MinusOne;
			Block = 0;
			TranslatedPos = Vector3I.MinusOne;
			Face = (BlockFace)6;
		}
		
		void TestAxis(float dAxis, ref float dist, BlockFace fAxis) {
			dAxis = Math.Abs(dAxis);
			if (dAxis >= dist) return;
			dist = dAxis; Face = fAxis;
		}
	}
}