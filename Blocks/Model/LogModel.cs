using System;

namespace ClassicalSharp.Blocks.Model {
	
	public class LogModel : CubeModel {		
		
		TextureRectangle[][] logRecs = new TextureRectangle[4][];
		public LogModel( Game game, byte block ) : base( game, block ) {
			TextureRectangle rec = atlas.GetTexRec( 3, 1 );
			TextureRectangle verRec = atlas.GetTexRec( 4, 1 );
			logRecs[0] = new [] { rec, rec, rec, rec, verRec, verRec };
			logRecs[3] = new [] { rec, rec, rec, rec, verRec, verRec };
			rec = atlas.GetTexRec( 8, 6 );
			logRecs[1] = new [] { rec, rec, rec, rec, verRec, verRec };
			rec = atlas.GetTexRec( 9, 6 );
			logRecs[2] = new [] { rec, rec, rec, rec, verRec, verRec };
		}
		
		public override void DrawFace( int face, byte meta, ref Neighbours state, ref int index,
		                              float x, float y, float z, VertexPos3fTex2fCol4b[] vertices, FastColour col ) {
			recs = logRecs[meta & 0x03];
			base.DrawFace( face, meta, ref state, ref index, x, y, z, vertices, col );
		}
	}
}
