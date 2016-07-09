// ClassicalSharp copyright 2014-2016 UnknownShadow200 | Licensed under MIT
using System;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Map;
using OpenTK;

namespace ClassicalSharp {

	public sealed class SmoothLightingMeshBuilder : ChunkMeshBuilder {
		
		protected override int StretchXLiquid( int xx, int countIndex, int x, int y, int z, int chunkIndex, byte block ) {
			if( OccludedLiquid( chunkIndex ) ) return 0;
			return 1;
		}
		
		protected override int StretchX( int xx, int countIndex, int x, int y, int z, int chunkIndex, byte block, int face ) {
			return 1;
		}
		
		protected override int StretchZ( int zz, int countIndex, int x, int y, int z, int chunkIndex, byte block, int face ) {
			return 1;
		}
	}
}