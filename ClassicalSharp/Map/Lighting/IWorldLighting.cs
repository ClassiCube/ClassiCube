// Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
using System;
using OpenTK;

#if USE16_BIT
using BlockID = System.UInt16;
#else
using BlockID = System.Byte;
#endif

namespace ClassicalSharp.Map {
	
	/// <summary> Manages lighting for each block in the world.  </summary>
	public abstract class IWorldLighting : IGameComponent {

		protected internal short[] heightmap;
		public int Outside, OutsideZSide, OutsideXSide, OutsideYBottom;
		protected int width, height, length;
		
		// Equivalent to
		// for x = startX; x < startX + 18; x++
		//    for z = startZ; z < startZ + 18; z++
		//       CalcHeightAt(x, maxY, z) if height == short.MaxValue
		// Except this function is a lot more optimised and minimises cache misses.
		public unsafe abstract void LightHint(int startX, int startZ, BlockID* mapPtr);
		
		/// <summary> Called when a block is changed, to update the lighting information. </summary>
		/// <remarks> Derived classes ***MUST*** mark all chunks affected by this lighting change
		/// as needing to be refreshed. </remarks>
		public abstract void OnBlockChanged(int x, int y, int z, BlockID oldBlock, BlockID newBlock);
		
		/// <summary> Discards all cached lighting information. </summary>
		public virtual void Refresh() { }
		
		
		/// <summary> Returns whether the block at the given coordinates is fully in sunlight. </summary>
		/// <remarks> *** Does NOT check that the coordinates are inside the map. *** </remarks>
		public abstract bool IsLit(int x, int y, int z);

		/// <summary> Returns the light colour of the block at the given coordinates. </summary>
		/// <remarks> *** Does NOT check that the coordinates are inside the map. *** </remarks>
		public abstract int LightCol(int x, int y, int z);

		/// <summary> Returns the light colour of the block at the given coordinates. </summary>
		/// <remarks> *** Does NOT check that the coordinates are inside the map. *** </remarks>
		public abstract int LightCol_ZSide(int x, int y, int z);
		

		public abstract int LightCol_Sprite_Fast(int x, int y, int z);		
		public abstract int LightCol_YTop_Fast(int x, int y, int z);
		public abstract int LightCol_YBottom_Fast(int x, int y, int z);
		public abstract int LightCol_XSide_Fast(int x, int y, int z);
		public abstract int LightCol_ZSide_Fast(int x, int y, int z);		
		
		public virtual void Dispose() { }
		public virtual void Reset(Game game) { }
		public virtual void OnNewMap(Game game) { }
		public virtual void OnNewMapLoaded(Game game) { }
		public virtual void Init(Game game) {  }
		public virtual void Ready(Game game) { }
	}
}
