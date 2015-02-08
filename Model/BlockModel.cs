using System;
using ClassicalSharp.Blocks.Model;
using ClassicalSharp.GraphicsAPI;
using ClassicalSharp.Renderers;

namespace ClassicalSharp.Model {

	public class BlockModel : IModel {
		
		byte block = (byte)BlockId.Air;
		FastColour[] colours;
		public BlockModel( Game window ) : base( window ) {
			Map map = window.Map;
			colours = new FastColour[] {
				map.SunlightXSide,
				map.SunlightXSide,
				map.SunlightZSide,
				map.SunlightZSide,
				map.SunlightYBottom,
				map.Sunlight,				
			};
		}
		
		protected override void DrawModelImpl( Entity entity, EntityRenderer renderer ) {
			graphics.Texturing = true;
			graphics.AlphaTest = true;
			block = Byte.Parse( entity.ModelName );
			if( block == 0 ) return;
			
			graphics.Bind2DTexture( window.TerrainAtlasTexId );
			IBlockModel model = window.BlockInfo.GetModel( block );
			
			int drawFlags = 0x3F;
			int verticesCount = 0;		
			Neighbours state = new Neighbours();
			for( int face = 0; face < 6; face++ ) {
				if( !model.HasFace( face ) ) {
					drawFlags &= ~( 1 << face );
					continue;
				}
				verticesCount += model.GetVerticesCount( face, 0, state, 0 );
			}
			
			int index = 0;
			VertexPos3fTex2fCol4b[] vertices = new VertexPos3fTex2fCol4b[verticesCount];
			for( int face = 0; face < 6; face ++ ) {
				if( ( drawFlags & ( 1 << face ) ) != 0 ) {
					model.DrawFace( face, 0, state, ref index, -0.5f, -0.5f, -0.5f, vertices, colours[face] );
				}
			}
			graphics.DrawVertices( DrawMode.Triangles, vertices, 6 * 6 );
		}
		
		public override void Dispose() {
		}
	}
}