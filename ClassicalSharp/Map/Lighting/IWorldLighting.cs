// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using OpenTK;

namespace ClassicalSharp.Map {
	
	/// <summary> Manages lighting for each block in the world.  </summary>
	public abstract class IWorldLighting : IGameComponent {

		internal short[] heightmap;
		
		// Equivalent to
		// for x = startX; x < startX + 18; x++
		//    for z = startZ; z < startZ + 18; z++
		//       CalcHeightAt(x, maxY, z) if height == short.MaxValue
		// Except this function is a lot more optimised and minimises cache misses.
		public unsafe abstract void LightHint(int startX, int startZ, byte* mapPtr);
		
		/// <summary> Returns the y coordinate of the highest block that is fully not in sunlight. </summary>
		/// <remarks> e.g. if cobblestone was at y = 5, this method would return 4. </remarks>
		public abstract int GetLightHeight(int x, int z);
		
		/// <summary> Updates the lighting for the block at that position, which may in turn affect other blocks. </summary>
		public abstract void UpdateLight(int x, int y, int z, byte oldBlock, byte newBlock);		
		
		/// <summary> Returns whether the given world coordinates are fully in sunlight. </summary>
		public abstract bool IsLit(int x, int y, int z);

		/// <summary> Returns whether the given world coordinates are fully in sunlight. </summary>
		/// <remarks> Does not check whether the coordinates are inside the map. </remarks>
		public abstract bool IsLitNoCheck(int x, int y, int z);
		
		/// <summary> Returns whether the given world coordinates are fully in sunlight. </summary>
		public bool IsLit(Vector3 p) {
			int x = Utils.Floor(p.X), y = Utils.Floor(p.Y), z = Utils.Floor(p.Z);
			return IsLit(x, y, z);
		}
		
		
		public virtual void Dispose() { }
		public virtual void Reset(Game game) { }
		public virtual void OnNewMap(Game game) { }
		public virtual void OnNewMapLoaded(Game game) { }
		public virtual void Init(Game game) {  }
		public virtual void Ready(Game game) { }
	}
}
