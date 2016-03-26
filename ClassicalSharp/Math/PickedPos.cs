// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
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
		public Vector3 IntersectPoint;
		
		/// <summary> Integer world coordinates of the block. </summary>
		public Vector3I BlockPos;
		
		/// <summary> Integer world coordinates of the neighbouring block that is closest to the client.</summary>
		public Vector3I TranslatedPos;
		
		/// <summary> Whether this instance actually has a selected block currently. </summary>
		public bool Valid = true;
		
		/// <summary> Face of the picked block that is closet to the client. </summary>
		public CpeBlockFace BlockFace;
		
		/// <summary> Block type of this selected block. </summary>
		public byte BlockType;
		
		/// <summary> Mark this as having a selected block, and 
		/// calculates the closest block face of the selected position. </summary>
		public void SetAsValid( int x, int y, int z, Vector3 min, Vector3 max, 
		                       byte block, Vector3 intersect ) {
			Min = min;
			Max = max;
			BlockPos = new Vector3I( x, y, z );
			Valid = true;
			BlockType = block;
			IntersectPoint = intersect;
			
			Vector3I normal = Vector3I.Zero;
			float dist = float.PositiveInfinity;
			TestAxis( intersect.X - Min.X, ref dist, -Vector3I.UnitX, ref normal, CpeBlockFace.XMin );
			TestAxis( intersect.X - Max.X, ref dist, Vector3I.UnitX, ref normal, CpeBlockFace.XMax );
			TestAxis( intersect.Y - Min.Y, ref dist, -Vector3I.UnitY, ref normal, CpeBlockFace.YMin );
			TestAxis( intersect.Y - Max.Y, ref dist, Vector3I.UnitY, ref normal, CpeBlockFace.YMax );
			TestAxis( intersect.Z - Min.Z, ref dist, -Vector3I.UnitZ, ref normal, CpeBlockFace.ZMin );
			TestAxis( intersect.Z - Max.Z, ref dist, Vector3I.UnitZ, ref normal, CpeBlockFace.ZMax );
			TranslatedPos = BlockPos + normal;
		}
		
		/// <summary> Mark this as not having a selected block. </summary>
		public void SetAsInvalid() {
			Valid = false;
			BlockPos = TranslatedPos = Vector3I.MinusOne;
			BlockFace = (CpeBlockFace)255;
			BlockType = 0;
		}
		
		void TestAxis( float dAxis, ref float dist, Vector3I nAxis, ref Vector3I normal, 
		                     CpeBlockFace fAxis) {
			dAxis = Math.Abs( dAxis );
			if( dAxis < dist ) {
				dist = dAxis;
				normal = nAxis;
				BlockFace = fAxis;
			}
		}
	}
}