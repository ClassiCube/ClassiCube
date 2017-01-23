// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

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
		public Vector3I TranslatedPos { get { return BlockPos + offsets[(byte)Face]; } }
		
		/// <summary> Whether this instance actually has a selected block currently. </summary>
		public bool Valid = true;
		
		/// <summary> Face of the picked block that is closet to the player. </summary>
		public BlockFace Face;
		
		/// <summary> Block ID of the picked block. </summary>
		public byte Block;
		
		/// <summary> Mark this as having a selected block, and
		/// calculates the closest face of the selected block's position. </summary>
		public void SetAsValid(int x, int y, int z, Vector3 min, Vector3 max,
		                       byte block, Vector3 intersect) {
			Min = min;
			Max = max;
			BlockPos = new Vector3I(x, y, z);
			Valid = true;
			Block = block;
			Intersect = intersect;
			
			float dist = float.PositiveInfinity;
			TestAxis(intersect.X - Min.X, ref dist, BlockFace.XMin);
			TestAxis(intersect.X - Max.X, ref dist, BlockFace.XMax);
			TestAxis(intersect.Y - Min.Y, ref dist, BlockFace.YMin);
			TestAxis(intersect.Y - Max.Y, ref dist, BlockFace.YMax);
			TestAxis(intersect.Z - Min.Z, ref dist, BlockFace.ZMin);
			TestAxis(intersect.Z - Max.Z, ref dist, BlockFace.ZMax);
		}
		
		/// <summary> Mark this as not having a selected block. </summary>
		public void SetAsInvalid() {
			Valid = false;
			BlockPos = Vector3I.MinusOne;
			Face = (BlockFace)6;
			Block = 0;
		}
		
		void TestAxis(float dAxis, ref float dist, BlockFace fAxis) {
			dAxis = Math.Abs(dAxis);
			if (dAxis >= dist) return;			
			dist = dAxis; Face = fAxis;
		}
		
		static Vector3I[] offsets = new Vector3I[] {
		    Vector3I.UnitX, -Vector3I.UnitX, Vector3I.UnitY, 
		    -Vector3I.UnitY, Vector3I.UnitZ, -Vector3I.UnitZ,
		    Vector3I.Zero,
		};
	}
}